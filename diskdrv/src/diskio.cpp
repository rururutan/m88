// ---------------------------------------------------------------------------
//	PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: diskio.cpp,v 1.2 1999/09/25 03:13:51 cisc Exp $

#include "headers.h"
#include "misc.h"
#include "diskio.h"

#define LOGNAME "DiskIO"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築破壊
//
DiskIO::DiskIO(const ID& id)
: Device(id)
{
}

DiskIO::~DiskIO()
{
}

// ---------------------------------------------------------------------------
//	Init
//
bool DiskIO::Init()
{
	Reset();
	return true;
}

void DiskIO::Reset(uint, uint)
{
	writebuffer = false;
	filename[0] = 0;
	IdlePhase();
}

// ---------------------------------------------------------------------------
//	コマンド
//
void DiskIO::SetCommand(uint, uint d)
{
	if (d != 0x84 || !writebuffer)
		file.Close();
	phase = idlephase;
	cmd = d;
	LOG1("\n[%.2x]", d);
	status |= 1;
	ProcCommand();
}

// ---------------------------------------------------------------------------
//	ステータス
//
uint DiskIO::GetStatus(uint)
{
	return status;
}

// ---------------------------------------------------------------------------
//	データセット
//
void DiskIO::SetData(uint, uint d)
{
	if (phase == recvphase || phase == argphase)
	{
		*ptr++ = d;
		if (--len <= 0)
		{
			status &= ~2;
			ProcCommand();
		}
	}
}

// ---------------------------------------------------------------------------
//	データげっと
//
uint DiskIO::GetData(uint)
{
	uint r = 0xff;
	if (phase == sendphase)
	{
		r = *ptr++;
		if (--len <= 0)
		{
			status &= ~(2|4);
			ProcCommand();
		}
	}
	return r;
}

// ---------------------------------------------------------------------------
//
//
void DiskIO::SendPhase(uint8* p, int l)
{
	ptr = p, len = l;
	phase = sendphase;
	status |= 2 | 4;
}

void DiskIO::ArgPhase(int l)
{
	ptr = arg, len = l;
	phase = argphase;
}

// ---------------------------------------------------------------------------
//
//
void DiskIO::RecvPhase(uint8* p, int l)
{
	ptr = p, len = l;
	phase = recvphase;
	status |= 2;
}

// ---------------------------------------------------------------------------
//
//
void DiskIO::IdlePhase()
{
	phase = idlephase;
	status = 0;
	file.Close();
}

// ---------------------------------------------------------------------------
//
//
void DiskIO::ProcCommand()
{
	switch (cmd)
	{
	case 0x80:	CmdSetFileName();	break;
	case 0x81:	CmdReadFile();		break;
	case 0x82:	CmdWriteFile();		break;
	case 0x83:	CmdGetError();		break;
	case 0x84:	CmdWriteFlush();	break;
	default:	IdlePhase();		break;
	}
}

// ---------------------------------------------------------------------------

void DiskIO::CmdSetFileName()
{
	switch (phase)
	{
	case idlephase:
		LOG0("SetFileName ");
		ArgPhase(1);
		break;

	case argphase:
		if (arg[0])
		{
			RecvPhase(filename, arg[0]);
			err = 0;
			break;
		}
		err = 56;
	case recvphase:
		filename[arg[0]] = 0;
		LOG1("Path=%s", filename);
		IdlePhase();
		break;
	}
}

// ---------------------------------------------------------------------------

void DiskIO::CmdReadFile()
{
	switch (phase)
	{
	case idlephase:
		writebuffer = false;
		LOG1("ReadFile(%s) - ", filename);
		if (file.Open((char*) filename, FileIO::readonly))
		{
			file.Seek(0, FileIO::end);
			size = Min(0xffff, file.Tellp());
			file.Seek(0, FileIO::begin);
			buf[0] = size & 0xff;
			buf[1] = (size >> 8) & 0xff;
			LOG1("%d bytes  ", size);
			SendPhase(buf, 2);
		}
		else
		{
			LOG0("failed");
			err = 53;
			IdlePhase();
		}
		break;

	case sendphase:
		if (size > 0)
		{
			int b = Min(1024, size);
			size -= b;
			if (file.Read(buf, b))
			{
				SendPhase(buf, b);
				break;
			}
			err = 64;
		}

		LOG0("success");
		IdlePhase();
		err = 0;
		break;
	}
}

// ---------------------------------------------------------------------------

void DiskIO::CmdWriteFile()
{
	switch (phase)
	{
	case idlephase:
		writebuffer = true;
		LOG1("WriteFile(%s) - ", filename);
		if (file.Open((char*) filename, FileIO::create))
			ArgPhase(2);
		else
		{
			LOG0("failed");
			IdlePhase(), err = 60;
		}
		break;

	case argphase:
		size = arg[0] + arg[1] * 256;
		if (size > 0)
		{
			LOG0("%d bytes ");
			length = Min(1024, size);
			size -= length;
			RecvPhase(buf, length);
		}
		else
		{
			LOG0("success");
			IdlePhase(), err = 0;
		}
		break;

	case recvphase:
		if (!file.Write(buf, length))
		{	
			LOG0("write error");
			IdlePhase(), err = 61;
		}
		if (size > 0)
		{
			length = Min(1024, size);
			size -= length;
			RecvPhase(buf, length);
		}
		else
			ArgPhase(2);
		break;
	}
}

void DiskIO::CmdWriteFlush()
{
	switch (phase)
	{
	case idlephase:
		LOG0("WriteFlush ");
		if (writebuffer)
		{
			if (length-len > 0)
				LOG1("%d bytes\n", length - len);
			file.Write(buf, length-len);
			writebuffer = false;
		}
		else
		{
			LOG0("failed\n");
			err = 51;
		}
		IdlePhase();
		break;
	}
}

// ---------------------------------------------------------------------------

void DiskIO::CmdGetError()
{
	switch (phase)
	{
	case idlephase:
		buf[0] = err;
		SendPhase(buf, 1);
		break;
		
	case sendphase:
		IdlePhase();
		break;
	}
}


// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor DiskIO::descriptor =
{
	DiskIO::indef, DiskIO::outdef
};

const Device::OutFuncPtr DiskIO::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &Reset),
	STATIC_CAST(Device::OutFuncPtr, &SetCommand),
	STATIC_CAST(Device::OutFuncPtr, &SetData),
};

const Device::InFuncPtr DiskIO::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, &GetStatus),
	STATIC_CAST(Device::InFuncPtr, &GetData),
};
