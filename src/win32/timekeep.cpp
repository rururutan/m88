// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//	$Id: timekeep.cpp,v 1.1 2002/04/07 05:40:11 cisc Exp $

#include "headers.h"
#include "timekeep.h"

// ---------------------------------------------------------------------------
//	ç\íz/è¡ñ≈
//
TimeKeeper::TimeKeeper()
{
	assert(unit > 0);

	LARGE_INTEGER li;
	if (QueryPerformanceFrequency(&li))
	{
		freq = (li.LowPart+unit*500) / (unit*1000);
		QueryPerformanceCounter(&li);
		base = li.LowPart;
	}
	else
	{
		freq = 0;
		timeBeginPeriod(1);			// ê∏ìxÇè„Ç∞ÇÈÇΩÇﬂÇÃÇ®Ç‹Ç∂Ç»Ç¢ÅcÇÁÇµÇ¢
		base = timeGetTime();
	}
	time = 0;
}

TimeKeeper::~TimeKeeper()
{
	if (!freq)
	{
		timeEndPeriod(1);
	}
}

// ---------------------------------------------------------------------------
//	éûä‘ÇéÊìæ
//
uint32 TimeKeeper::GetTime()
{
	if (freq)
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		uint32 dc = li.LowPart - base;
		time += dc / freq;
		base = li.LowPart - dc % freq;
		return time;
	}
	else
	{
		time = timeGetTime();
		return time * unit;
	}
}
