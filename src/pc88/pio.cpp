// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  FDIF 用 PIO (8255) のエミュレーション
//	・8255 のモード 0 のみエミュレート
// ---------------------------------------------------------------------------
//	$Id: pio.cpp,v 1.2 1999/03/24 23:27:13 cisc Exp $

#include "headers.h"
#include "pc88/pio.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	Reset
//
void PIO::Reset()
{
	port[0] = port[1] = port[2] = 0;
	SetCW(0);
}

void PIO::SetCW(uint data)
{
	// Control Word
	if (!(data & 0x80))
	{
		if (data & 0x01)
			port[2] |= 1 << ((data >> 1) & 7);
		else
			port[2] &= ~(1 << ((data >> 1) & 7));
	}
	else
	{
		readmask[0] = (data & 0x10 ? 0xff : 0x00);
		readmask[1] = (data & 0x02 ? 0xff : 0x00);
		readmask[2] = (data & 0x08 ? 0xf0 : 0x00)
			        | (data & 0x01 ? 0x0f : 0x00);
		port[0] = port[1] = port[2] = 0;
	}
}

