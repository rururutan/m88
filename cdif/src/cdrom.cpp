// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: cdrom.cpp,v 1.2 1999/11/26 10:12:47 cisc Exp $

#include "headers.h"
#include "cdrom.h"
#include "aspi.h"
#include "aspidef.h"
#include "cdromdef.h"

#define LOGNAME	"cdrom"
#include "diag.h"

#define SHIFT

#ifdef SHIFT
static bool shift = false;
#endif

// --------------------------------------------------------------------------
//	構築
//
CDROM::CDROM()
{
	aspi = 0;
	ntracks = 0;
	memset(track, 0xff, sizeof(track));
	LOG0("construct\n");
}

// --------------------------------------------------------------------------
//	破棄
//
CDROM::~CDROM()
{
	delete aspi;
}


// --------------------------------------------------------------------------
//	初期化
//
bool CDROM::Init()
{
	if (!aspi)
		aspi = new ASPI;
	if (aspi)
	{
		LOG0("init\n");
		if (FindDrive())
			return true;
		delete aspi;
		aspi = 0;
	}
	return false;
}

// --------------------------------------------------------------------------
//	ドライブを見つける
//
bool CDROM::FindDrive()
{
	for (uint a=0; a<aspi->GetNHostAdapters(); a++)
	{
		uint ntargets;
		if (aspi->InquiryAdapter(a, &ntargets, 0))
		{
			LOG2("Adapter %d : %d IDs\n", a, ntargets);
			for (uint i=0; i<ntargets; i++)
			{
				int type = aspi->GetDeviceType(a, i, 0);
				if (type == 5)
				{
					LOG2("CDROM on %d:%d\n", a, i);
					ha = a, id = i;
					return true;
				}
			}
		}
	}
	return false;
}

// --------------------------------------------------------------------------
//	TOC を読み込む
//
int CDROM::ReadTOC()
{
	CDB_ReadTOC cdb;
	TOC<100> toc;
	int r;
	
	memset(track, 0xff, sizeof(track));
	memset(&cdb, 0, sizeof(cdb));

	cdb.id = CD_READ_TOC;
	
	// トラック数と Track1 の MSF を取得
	cdb.flags = 2;
	cdb.length = 12;

	LOG0("Read TOC ");
	for (int i=0; i<2; i++)
	{
		r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb), ASPI::in, &toc, 12);
		if (r >= 0)
			break;
	}
	if (r < 0)
	{
		LOG0("failed\n");
		return 0;
	}

	r = toc.entry[0].addr;
	trstart = BCDtoN((r >> 16) & 0xff) * 75 * 60 
		    + BCDtoN((r >>  8) & 0xff) * 75
			+ BCDtoN((r >>  0) & 0xff);
	
	LOG3("[%d]-[%d] (%d)\n", toc.header.start, toc.header.end, trstart);
//	printf("[%d]-[%d]\n", toc.header.start, toc.header.end);

	// 各トラックの位置を取得
	int start = toc.header.start;
	int end = toc.header.end;
	int tsize = 4 + (end - start + 2) * 8;
	ntracks = end;
	
	cdb.flags = 0;
	cdb.length = tsize;
	r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb), ASPI::in, &toc, tsize);
	if (r < 0)
	{
		LOG0("failed\n");
		return 0;
	}

	// テーブルに登録
	Track* tr = track + start - 1;
#ifdef SHIFT
	shift = (toc.entry[1].addr == 13578);
#endif
	for (int t=0; t<end-start+2 && start+t < 100; t++, tr++)
	{
		tr->control = toc.entry[t].control;
		tr->addr = toc.entry[t].addr;
#ifdef SHIFT
		if (shift && tr->addr >= 13578)
			tr->addr -= 228;
#endif
		LOG3("Track %.2d: %6d %.2x\n", start + t, tr->addr, tr->control);
	}
	LOG0("\n");

	return end;
}

// --------------------------------------------------------------------------
//	トラックの再生
//
bool CDROM::PlayTrack(int t, bool one)
{
	if (t < 1 || ntracks < t)
		return false;
	Track* tr = &track[t-1];
	
	// 有効なトラックか？
	if (tr->addr == ~0)
		return false;
	
	// オーディオトラックか?
	if (tr->control & 0x04)
		return false;

	CDB_PlayAudio cdb;
	cdb.id = CD_PLAY_AUDIO;
	cdb.flags = 0;
	cdb.start = tr->addr;
#ifdef SHIFT
	if (shift && tr->addr >= 13550)
		cdb.start = tr->addr + 228;
#endif
	cdb.length = (one ? tr[1] : track[ntracks]).addr - tr[0].addr;
	cdb.rsvd = 0;
	cdb.control = 0;
	
//	printf("playcd %d (%d)\n", tr->addr, tr[1].addr - tr[0].addr);
	int r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb));
//	printf("r = %d\n", r);
	if (r < 0)
		return false;
	return true;
}

// --------------------------------------------------------------------------
//	トラックの再生
//
bool CDROM::PlayAudio(uint begin, uint stop)
{
	if (stop < begin)
		return false;
	
	CDB_PlayAudio cdb;
	cdb.id = CD_PLAY_AUDIO;
	cdb.flags = 0;
	cdb.start = begin;
#ifdef SHIFT
	if (shift && begin >= 13550)
		cdb.start = begin + 228;
#endif
	cdb.length = stop - begin;
	cdb.rsvd = 0;
	cdb.control = 0;
	
	int r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb));
	if (r < 0)
		return false;
	return true;
}

// --------------------------------------------------------------------------
//	サブチャンネルの取得
//
bool CDROM::ReadSubCh(uint8* dest, bool msf)
{
	CDB_ReadSubChannel cdb;
	memset(&cdb, 0, sizeof(cdb));

	cdb.id = CD_READ_SUBCH;
	cdb.flag1 = msf ? 2 : 0;
	cdb.flag2 = 0x40;
	cdb.format = 1;
	cdb.length = 16;

	int r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb), ASPI::in, dest, 16);
	if (r < 0)
		return false;
	return true;
}

// --------------------------------------------------------------------------
//	ポーズ
//
bool CDROM::Pause(bool pause)
{
	CDB_PauseResume cdb;
	memset(&cdb, 0, sizeof(cdb));

	cdb.id = CD_PAUSE_RESUME;
	cdb.flag = pause ? 0 : 1;

	int r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb));
	if (r < 0)
		return false;
	return true;
}

// --------------------------------------------------------------------------
//	停止
//
bool CDROM::Stop()
{
	CDB_StartStopUnit cdb;
	memset(&cdb, 0, sizeof(cdb));

	cdb.id = SCSI_StartStopUnit;
	cdb.flag2 = 0;

	int r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb));
	if (r < 0)
		return false;
	return true;
}

// --------------------------------------------------------------------------
//	CD 上のセクタを読み出す
//
bool CDROM::Read(uint sector, uint8* dest, int length)
{
	CDB_Read cdb;

	cdb.id = 0x28;
	cdb.flag = 0;
	cdb.addr = sector;
#ifdef SHIFT
	if (shift && sector >= 13550)
		cdb.addr = sector + 228;
#endif
	cdb.rsvd = 0;
	cdb.blocks = 1;
	cdb.control = 0;

	int r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb), ASPI::in, dest, length);
//	printf("r = %.4x", r);
	if (r < 0)
		return false;
	return true;
}

// --------------------------------------------------------------------------
//	2340 バイト読む
//
bool CDROM::Read2(uint sector, uint8* dest, int length)
{
	CDB_ReadCD cdb;
	memset(&cdb, 0, sizeof(cdb));

	cdb.id = CD_READ;
	cdb.flag1 = 0;
	cdb.addr = sector;
#ifdef SHIFT
	if (shift && sector >= 13550)
		cdb.addr = sector + 228;
#endif
	cdb.length = 1;
	cdb.flag2 = 0x78;
	cdb.subch = 0;

	int r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb), ASPI::in, dest, length);
	if (r < 0)
		return false;
	return true;
}

// --------------------------------------------------------------------------
//	CD-DA セクタの読み込み
//
bool CDROM::ReadCDDA(uint sector, uint8* dest, int length)
{
	CDB_ReadCD cdb;
	memset(&cdb, 0, sizeof(cdb));

	cdb.id = CD_READ;
	cdb.flag1 = 0x04;
	cdb.addr = sector;
#ifdef SHIFT
	if (shift && sector >= 13550)
		cdb.addr = sector + 228;
#endif
	cdb.length = (length + 2351) / 2352;
	cdb.flag2 = 0x10;
	cdb.subch = 0;

	int r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb), ASPI::in, dest, length);
	if (r < 0)
		return false;
	return true;
}

// --------------------------------------------------------------------------
//	メディアがドライブにあるか？
//
bool CDROM::CheckMedia()
{
	CDB_TestUnitReady cdb;
	memset(&cdb, 0, sizeof(cdb));

	cdb.id = SCSI_TestUnitReady;

	int r = aspi->ExecuteSCSICommand(ha, id, 0, &cdb, sizeof(cdb));
	if (r != 0)
		return false;
	return true;
}
