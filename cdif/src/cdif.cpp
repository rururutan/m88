// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	CD-ROM インターフェースの実装
// ----------------------------------------------------------------------------
//	$Id: cdif.cpp,v 1.2 1999/10/10 01:39:00 cisc Exp $

#include "headers.h"
#include "cdif.h"

#define LOGNAME "cdif"
#include "diag.h"

using namespace PC8801;

// ----------------------------------------------------------------------------
//	構築
//
CDIF::CDIF(const ID& id)
: Device(id)
{
}

// ----------------------------------------------------------------------------
//	破棄
//
CDIF::~CDIF()
{
}

// ----------------------------------------------------------------------------
//	初期化
// 
bool CDIF::Init(IDMAAccess* _dmac)
{
	dmac = _dmac;
	if (!cdrom.Init())
		return false;
	if (!cd.Init(&cdrom, this, (void (Device::*)(int)) &CDIF::Done))
		return false;
	enable = false;
	active = false;
	return true;
}

bool CDIF::Enable(bool f)
{
	enable = f;
	return true;
}

// ----------------------------------------------------------------------------
//	M88 のリセット
//	BASICMODE の bit 6 で判定
//
void IOCALL CDIF::SystemReset(uint, uint d)
{
	Enable((d & 0x40) != 0);
}

// ----------------------------------------------------------------------------
//	リセット
//
void CDIF::Reset()
{
	LOG0("Reset!\n");
	status = 0;
	data = 0;
	phase = idlephase;

	playmode = 0;
	addrs = 0;

	stillmode = 0;
	cd.SendCommand(CDControl::stop, true);
}

// ----------------------------------------------------------------------------
//	データ交換
//
void CDIF::DataOut()
{
	int s = status & 0x38;
	status &= ~(0x40 | 0x38);
	switch (s)
	{
	case 0x00:	// データ送信モード
		*ptr++ = data;
		LOG1("(%.2x)", data);

		if (--length > 0)
			status |= 0x00 | 0x40; 
		else
		{
			LOG0("\n");
			ProcessCommand();
		}
		break;

	case 0x10:
		switch (phase)
		{
		case cmd2phase:
			length = 1 + (data < 0x80 ? 5 : 9);
			phase = paramphase;
			ptr = cmdbuf;
			LOG0("Command: ");

		case paramphase:
			LOG1("[%.2x]", data);
			*ptr++ = data;
			if (--length > 0)
			{
				status |= 0x40 | 0x10;
			}
			else
			{
				LOG0(" - ");
				status &= ~0x38;
				phase = execphase;
				ProcessCommand();
			}
			break;
		}
		break;
	}
}

void CDIF::DataIn()
{
	int s = status & 0x38;
	status &= ~(0x40 | 0x38);
	switch (s)
	{
	case 0x08:	// データ受信モード
		if (length-- > 0)
		{
			status |= 0x08 | 0x40; 
			data = *ptr++;
		}
		else
			ResultPhase(rslt, stat);
		break;

	case 0x18:	// 結果受信モード
		LOG0(">Status\n");
		data = 0;
		phase = statusphase;
		status |= 0x38 | 0x40;
		break;

	case 0x38:	// 終了ステータス受信モード
		LOG0(">Idle\n");
		data = 0;
		phase = idlephase;
		status = 0;
		break;
	}
}

void CDIF::SendPhase(int bytes, int r, int s)
{
	rslt = r, stat = s;
	
	LOG0(">SendPhase\n");
	phase = sendphase;
	length = bytes - 1;
	status |= 0x08 | 0x40;
	ptr = datbuf;
	data = *ptr++;
}

void CDIF::RecvPhase(int bytes)
{
	LOG0(">RecvPhase\n");
	phase = recvphase;
	length = bytes;
	status |= 0x00 | 0x40;
	ptr = datbuf;
}

void CDIF::ResultPhase(int res, int st)
{
	LOG0(">Result\n");
	data = res;
	stat = st;
	status |= 0x18 | 0x40;
	phase = resultphase;
}

// ----------------------------------------------------------------------------
//
//
void CDIF::SendCommand(uint a, uint b, uint c)
{
	phase = waitphase;
	cd.SendCommand(a, b, c);
}

void CDIF::Done(int ret)
{
	LOG3("[done(%d:%d:%d)]", cmdbuf[0], phase, ret);
	rslt = ret;
	if (phase == waitphase)
		ProcessCommand();
}

// ----------------------------------------------------------------------------
//	コマンド処理
//
void CDIF::ProcessCommand()
{
	switch (cmdbuf[0])
	{
	case 0x00:	CheckDriveStatus();	break;
	case 0x08:	ReadSector();		break;
	case 0x15:	SetReadMode();		break;

	case 0xd8:	TrackSearch();		break;
	case 0xd9:	PlayStart();		break;
	case 0xda:	PlayStop();			break;
	case 0xdd:	ReadSubcodeQ();		break;
	case 0xde:	ReadTOC();			break;

	default:
		LOG0("unknown\n");
		ResultPhase(0, 0);
		break;
	}
}

// ----------------------------------------------------------------------------
//	セクタを読む
//
void CDIF::ReadSector()
{
	switch (phase)
	{
		int n;

	case execphase:
		sector = (((cmdbuf[1] << 8) + cmdbuf[2]) << 8) + cmdbuf[3];
		LOG1("Read Sector (%d)\n", sector);
//		statusdisplay.Show(90, 0, "Read Sector (%d)", sector);
		length = retrycount+1;
		rslt = 0;

	case waitphase:
		if (!rslt)
		{
			if (length-- > 0)
			{
				LOG2("(%2d) Read#%d\n", rslt, retrycount - length + 1);
				SendCommand(readmode ? CDControl::read2 : CDControl::read1, sector, (uint) tmpbuf);
				break;
			}
			ResultPhase(0, 0);
			break;
		}
		n = dmac->RequestWrite(1, tmpbuf, readmode ? 2340 : 2048);
		LOG1("DMA: %d bytes\n", n);
		ResultPhase(0, 0);
		break;
	}
}

// ----------------------------------------------------------------------------
//	セクタ読み込みモードの設定
//
void CDIF::SetReadMode()
{
	switch (phase)
	{
	case execphase:
		LOG1("Set Read Mode (%d)\n", cmdbuf[4]);
		RecvPhase(11);
		break;

	case recvphase:
		retrycount = datbuf[10];
		readmode = datbuf[4];
		ResultPhase(0, 0);
		break;
	}
}

// ----------------------------------------------------------------------------
//	トラックサーチ
//
void CDIF::TrackSearch()
{
	switch (phase)
	{
		uint addre;

	case execphase:
		addrs = GetPlayAddress();
//		addre = cmdbuf[1] & 1 ? cdrom.GetTrackInfo(0)->addr : addrs+1;
		addre = cdrom.GetTrackInfo(0)->addr;
		
		LOG2("Track Search (%d - %d)\n", addrs, addre);
//		statusdisplay.Show(90, 0, "Search Track (%d)", addrs);
		SendCommand(CDControl::playaudio, addrs, addre);
		if (cmdbuf[1] & 1)
			stillmode = 2;
		else
			stillmode = 0;
		break;
	
	case waitphase:
		if (stillmode == 0)
		{
			stillmode = 2;
			SendCommand(CDControl::pause);
			break;
		}
		ResultPhase(0, 0);
		break;
	}
}

// ----------------------------------------------------------------------------
//
//
void CDIF::PlayStart()
{
	switch (phase)
	{
		uint addre;

	case execphase:
		addre = GetPlayAddress();
		LOG2("Audio Play Start (%d - %d)\n", addrs, addre);
//		statusdisplay.Show(90, 0, "Play Audio (%d - %d)", addrs, addre);
		SendCommand(CDControl::playaudio, addrs, addre);
		break;
		
	case waitphase:
		ResultPhase(0, 0);
		break;
	}
}

// ----------------------------------------------------------------------------
//
//
void CDIF::PlayStop()
{
	switch (phase)
	{
	case execphase:
		addrs = cd.GetTime();
		SendCommand(CDControl::pause);
//		statusdisplay.Show(90, 0, "Pause");
		stillmode = 1;
		break;
		
	case waitphase:
		ResultPhase(0, 0);
		break;
	}
}

// ----------------------------------------------------------------------------
//	サブコード読込
//
void CDIF::ReadSubcodeQ()
{
	switch (phase)
	{
	case execphase:
		LOG0("Read Subcode-Q\n");
		SendCommand(CDControl::readsubcodeq, (uint) tmpbuf);
		break;

	case waitphase:
		switch (tmpbuf[1])
		{
		case 0x11: datbuf[0] = 0; break;
		case 0x12: datbuf[0] = stillmode; break;
		case 0x13: datbuf[0] = 3; break;
		default:   datbuf[0] = 3; break;
		}
		datbuf[1] = tmpbuf[ 5] & 0x0f;
		datbuf[2] = NtoBCD(tmpbuf[ 6]);
		datbuf[3] = NtoBCD(tmpbuf[ 7]);
		datbuf[4] = NtoBCD(tmpbuf[13]);
		datbuf[5] = NtoBCD(tmpbuf[14]);
		datbuf[6] = NtoBCD(tmpbuf[15]);
		datbuf[7] = NtoBCD(tmpbuf[ 9]);
		datbuf[8] = NtoBCD(tmpbuf[10]);
		datbuf[9] = NtoBCD(tmpbuf[11]);
		SendPhase(Min(cmdbuf[1], 10), 0, 0);
		break;
	}
}

// ----------------------------------------------------------------------------
//	ドライブ状態の取得
//
void CDIF::CheckDriveStatus()
{
	switch (phase)
	{
	case execphase:
		LOG0("Check Drive Status");
		SendCommand(CDControl::checkdisk);
		break;

	case waitphase:
		LOG2("result : %d (%d)\n", rslt, cdrom.GetNumTracks());
		ResultPhase(0, 0);
		break;
	}
}

// ----------------------------------------------------------------------------
//	READ TOC
//
void CDIF::ReadTOC()
{
	int t = 0;
	switch (phase)
	{
	case execphase:
		LOG0("READ TOC - ");
		
		switch (cmdbuf[1])
		{
		case 0x00:
			SendCommand(CDControl::readtoc);
			break;

		case 0x02:
			t = (cmdbuf[2] / 16) * 10 + (cmdbuf[2] & 0x0f);
		case 0x01:
			if (t <= cdrom.GetNumTracks())
			{
				const CDROM::Track* tr = cdrom.GetTrackInfo(t);
				if (t)
					LOG2("Track %d(%p) ", t, tr);
				else
					LOG0("ReadOut");
				
				CDROM::MSF msf = cdrom.ToMSF(tr->addr);
				datbuf[0] = msf.min;
				datbuf[1] = msf.sec;
				datbuf[2] = msf.frame;
				datbuf[3] = t ? tr->control & 0x0f : 0;
				LOG5(" : %8d/%.2x:%.2x.%.2x %.2x\n", tr->addr, msf.min, msf.sec, msf.frame, t ? tr->control : 0);
				SendPhase(4, 0, 0);
				break;
			}
			ResultPhase(0, 0);
			break;
		
		default:
			ResultPhase(0, 0);
			break;
		}
		break;

	case waitphase:
		rslt = cdrom.GetNumTracks();
//		statusdisplay.Show(90, 0, "Read TOC - %d tracks", rslt);
		LOG1("GetNumTracks (%d)\n", rslt);
		for (t=0; t<rslt; t++)
		{
			const CDROM::Track* tr = cdrom.GetTrackInfo(t);
			LOG2("  %d: %d\n", t, tr->addr);
		}
		if (rslt)
			datbuf[0] = 1, datbuf[1] = NtoBCD(rslt);
		else
			datbuf[0] = 0, datbuf[1] = 0;
		datbuf[2] = 0; datbuf[3] = 0;
		SendPhase(4, 0, 0);
		break;
	}
}

// ----------------------------------------------------------------------------
//	再生コマンドのアドレスを取得
//
uint CDIF::GetPlayAddress()
{
	switch (cmdbuf[9] & 0xc0)
	{
		CDROM::MSF msf;
		int t;
	
	case 0x00:
		return (((((cmdbuf[2] << 8) + cmdbuf[3]) << 8) + cmdbuf[4]) << 8) + cmdbuf[5];
	
	case 0x40:
		msf.min = cmdbuf[2];
		msf.sec = cmdbuf[3];
		msf.frame = cmdbuf[4];
		return cdrom.ToLBA(msf);

	case 0x80:
		t = BCDtoN(cmdbuf[2]);
		if (0 < t && t <= cdrom.GetNumTracks())
			return cdrom.GetTrackInfo(t)->addr;
	default:
		return cdrom.GetTrackInfo(0)->addr;
	}
}

// ----------------------------------------------------------------------------
//	I/O
//
void IOCALL CDIF::Out90(uint, uint d)
{
	LOG1("O[90] <- %.2x\n", d);
	if (d & 1)
	{
		if (active && data == 0x81)
		{
			LOG0("Command_A\n");
			status |= 0x40 | 0x10 | 1;
			status &= ~(0x80 | 0x38);
			phase = cmd1phase;
		}
	}
	else
	{
		status &= ~1;
		if (phase == cmd1phase)
		{
			LOG0("Command_B\n");
			phase = cmd2phase;
			status = (status & ~0x78) | 0x80 | 0x50;
		}
	}
}

uint IOCALL CDIF::In90(uint)
{
//	LOG1("I[90] -> %.2x\n", status);
	return status;
}

// ----------------------------------------------------------------------------
//	データポート
//	
void IOCALL CDIF::Out91(uint, uint d)
{
//	LOG1("O[91] <- %.2x (DATA)\n", d);
	data = d;
	if (status & 0x80)
		DataOut();
}

uint IOCALL CDIF::In91(uint)
{
	LOG1("I[91] -> %.2x\n", data);
	uint r = data;
	if (status & 0x80)
		DataIn();
	return r;
}

void IOCALL CDIF::Out94(uint, uint d)
{
	LOG1("O[94] <- %.2x\n", d);
	if (d & 0x80)
	{
		Reset();
	}
}

void IOCALL CDIF::Out97(uint, uint d)
{
//	cd.SendCommand(CDControl::playtrack, d);
	LOG1("O[97] <- %.2x\n", d);
}

void IOCALL CDIF::Out99(uint, uint d)
{
//	LOG1("O[99] <- %.2x\n", d);
}

void IOCALL CDIF::Out9f(uint, uint d)
{
//	cd.SendCommand(CDControl::readtoc);
	LOG1("O[9f] <- %.2x", d);
	if (enable)
	{
		active = d & 1;
		LOG1("  CD-ROM drive %s.\n", active ? "activated" : "deactivated");
	}
}

uint IOCALL CDIF::In92(uint)
{
	LOG1("I[92] -> %.2x\n", 0);
	return 0;
}

uint IOCALL CDIF::In93(uint)
{
	LOG1("I[93] -> %.2x\n", 0);
	return 0;
}

uint IOCALL CDIF::In96(uint)
{
	LOG1("I[96] -> %.2x\n", 0);
	return 0;
}

uint IOCALL CDIF::In99(uint)
{
	LOG1("I[99] -> %.2x\n", 0);
	return 0;
}

uint IOCALL CDIF::In9b(uint)
{
//	LOG1("I[9b] -> %.2x\n", 0);
	return 60;
}

uint IOCALL CDIF::In9d(uint)
{
//	LOG1("I[9d] -> %.2x\n", 0);
	return 60;
}

// ---------------------------------------------------------------------------
//	再生モード
//
void IOCALL CDIF::Out98(uint, uint d)
{
	LOG1("O[98] <- %.2x\n", d);
	playmode = d;
}

uint IOCALL CDIF::In98(uint)
{
	if (enable)
		clk = ~clk;
	
	uint r = (clk & 0x80) | (playmode & 0x7f);
	LOG1("I[98] -> %.2x\n", r);
	return r;
}

// ---------------------------------------------------------------------------
//	状態データのサイズ
//
uint IFCALL CDIF::GetStatusSize()
{
	return sizeof(Snapshot);
}

// ---------------------------------------------------------------------------
//	状態保存
//
bool IFCALL CDIF::SaveStatus(uint8* s)
{
	Snapshot* ss = (Snapshot*) s;
	
	ss->rev			= ssrev;
	ss->phase		= phase;
	ss->status		= status;
	ss->data		= data;
	ss->playmode	= playmode;
	ss->retrycount	= retrycount;
	ss->stillmode	= stillmode;
	ss->rslt		= rslt;
	ss->sector		= sector;
	ss->ptr			= ptr - cmdbuf;
	ss->length		= length;
	ss->addrs		= addrs;

	memcpy(ss->buf, cmdbuf, 16+16+2340);
	return true;
}

bool IFCALL CDIF::LoadStatus(const uint8* s)
{
	const Snapshot* ss = (const Snapshot*) s;
	if (ss->rev != ssrev)
		return false;

	phase		= ss->phase;
	status		= ss->status;
	data		= ss->data;
	playmode	= ss->playmode;
	retrycount	= ss->retrycount;
	stillmode	= ss->stillmode;
	rslt		= ss->rslt;
	sector		= ss->sector;
	ptr			= cmdbuf + ss->ptr;
	length		= ss->length;
	addrs		= ss->addrs;

	memcpy(cmdbuf, ss->buf, 16+16+2340);

	return true;
}

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor CDIF::descriptor = { indef, outdef };

const Device::OutFuncPtr CDIF::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &SystemReset),
	STATIC_CAST(Device::OutFuncPtr, &Out90),
	STATIC_CAST(Device::OutFuncPtr, &Out91),
	STATIC_CAST(Device::OutFuncPtr, &Out94),
	STATIC_CAST(Device::OutFuncPtr, &Out97),
	STATIC_CAST(Device::OutFuncPtr, &Out98),
	STATIC_CAST(Device::OutFuncPtr, &Out99),
	STATIC_CAST(Device::OutFuncPtr, &Out9f),
};

const Device::InFuncPtr CDIF::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, &In90),
	STATIC_CAST(Device::InFuncPtr, &In91),
	STATIC_CAST(Device::InFuncPtr, &In92),
	STATIC_CAST(Device::InFuncPtr, &In93),
	STATIC_CAST(Device::InFuncPtr, &In96),
	STATIC_CAST(Device::InFuncPtr, &In98),
	STATIC_CAST(Device::InFuncPtr, &In99),
	STATIC_CAST(Device::InFuncPtr, &In9b),
	STATIC_CAST(Device::InFuncPtr, &In9d),
};
