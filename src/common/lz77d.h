// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 2000.
// ---------------------------------------------------------------------------
//	$Id: lz77d.h,v 1.1 2000/01/10 08:25:05 cisc Exp $

#pragma once

#include "types.h"

class LZ77Dec
{
public:
	bool Decode(uint8* dest, int destsize, const uint8* src);
};

