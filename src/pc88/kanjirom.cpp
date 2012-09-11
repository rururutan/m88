// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  äøéö ROM
// ---------------------------------------------------------------------------
//	$Id: kanjirom.cpp,v 1.6 2000/02/29 12:29:52 cisc Exp $

#include "headers.h"
#include "file.h"
#include "pc88/kanjirom.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	ç\íz/è¡ñ≈
// ---------------------------------------------------------------------------

KanjiROM::KanjiROM(const ID& id) : Device(id)
{
	image = 0;
	adr = 0;
}

KanjiROM::~KanjiROM()
{
	delete[] image;
}

// ---------------------------------------------------------------------------
//	èâä˙âª
//
bool KanjiROM::Init(const char* filename)
{
	if (!image)
		image = new uint8[0x20000];
	if (!image)
		return false;
	memset(image, 0xff, 0x20000);

	FileIO file(filename, FileIO::readonly);
	if (file.GetFlags() & FileIO::open)
	{
		file.Seek(0, FileIO::begin);
		file.Read(image, 0x20000);
	}
	return true;
}

// ---------------------------------------------------------------------------
//	I/O
//
void IOCALL KanjiROM::SetL(uint, uint d)
{
	adr = (adr & ~0xff) | d;
}

void IOCALL KanjiROM::SetH(uint, uint d)
{
	adr = (adr & 0xff) | (d * 0x100);
}

uint IOCALL KanjiROM::ReadL(uint)
{
	return image[adr * 2 + 1];
}

uint IOCALL KanjiROM::ReadH(uint)
{
	return image[adr * 2];
}

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor KanjiROM::descriptor =
{
	KanjiROM::indef, KanjiROM::outdef
};

const Device::OutFuncPtr KanjiROM::outdef[] =
{
	STATIC_CAST(Device::OutFuncPtr, &SetL),
	STATIC_CAST(Device::OutFuncPtr, &SetH),
};

const Device::InFuncPtr KanjiROM::indef[]  =
{
	STATIC_CAST(Device::InFuncPtr, &ReadL),
	STATIC_CAST(Device::InFuncPtr, &ReadH),
};
