// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	$Id: draw.h,v 1.7 2000/02/11 00:41:52 cisc Exp $

#pragma once

#include "types.h"
#include "misc.h"

// ---------------------------------------------------------------------------

class Draw
{
public:
	struct Palette
	{
		uint8 red, green, blue, rsvd;
	};

	struct Region
	{
		void Reset() { top = left = 32767; bottom = right = -1; }
		bool Valid() { return top <= bottom; }
		void Update(int l, int t, int r, int b)
		{
			left = Min(left, l), right  = Max(right , r);
			top  = Min(top,  t), bottom = Max(bottom, b);
		}
		void Update(int t, int b)
		{
			Update(0, t, 640, b);
		}

		int left,  top;
		int right, bottom;
	};

	enum Status
	{
		readytodraw		= 1 <<  0,		// 更新できることを示す
		shouldrefresh	= 1 <<  1,		// DrawBuffer をまた書き直す必要がある
		flippable		= 1 <<  2,		// flip が実装してあることを示す
	};

public:
	Draw() {}
	virtual ~Draw() {}

	virtual bool Init(uint width, uint height, uint bpp) = 0;
	virtual bool Cleanup() = 0;

	virtual bool Lock(uint8** pimage, int* pbpl) = 0;
	virtual bool Unlock() = 0;
	
	virtual uint GetStatus() = 0;
	virtual void Resize(uint width, uint height) = 0;
	virtual void DrawScreen(const Region& region) = 0;
	virtual void SetPalette(uint index, uint nents, const Palette* pal) = 0;
	virtual void Flip() {}
	virtual bool SetFlipMode(bool) = 0;
};

