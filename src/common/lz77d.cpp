// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 2000.
// ---------------------------------------------------------------------------
//	$Id: lz77d.cpp,v 1.1 2000/01/10 08:25:04 cisc Exp $

#include "headers.h"
#include "lz77d.h"

#define GetBit() (r=(bit>>bc)&1, bc--||(bit=*(uint32*)src,src+=4,bc=31), r)

bool LZ77Dec::Decode(uint8* dest, int destsize, const uint8* src)
{
	uint r;

	uint8* dtop = dest + destsize;
	
	uint bit = *(uint32*)src;
	uint bc = 31;
	src += 4;
	
	while (dest < dtop)
	{
		if (GetBit())
		{
			*dest++ = *src++;
		}
		else
		{
			int x = *src++;
			int len;
			if (!GetBit())
			{
				if (!x)
					return dest == dtop;

				x |= ~0xff;
				*dest++ = dest[x];
				*dest++ = dest[x];
				*dest++ = dest[x];
			}
			else
			{
				uint y = ~1 | GetBit();
				if (!GetBit())
				{
					if (GetBit())
					{
						y &= ~2;
					}
					else
					{
						y = y * 2 + GetBit();
						y = y * 2 + GetBit();
						y = y * 2 + GetBit();
					}
				}
				x |= y << 8;
				if (GetBit()) len = 3;
				else if (GetBit()) len = 4;
				else if (GetBit()) len = 5;
				else if (GetBit()) len = 6;
				else if (GetBit()) len = 7 + GetBit();
				else if (!GetBit())
				{
					len = GetBit();
					len = len * 2 + GetBit();
					len = len * 2 + GetBit() + 9;
				}
				else
					len = *src++ + 17;

				if (dest + len > dtop)
					return false;

				for (; len>0; len--)
					*dest++ = dest[x];
			}
		}
	}

	return !GetBit() && !*src++ && !GetBit();
}
