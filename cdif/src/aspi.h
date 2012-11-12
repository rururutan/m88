// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: aspi.h,v 1.1 1999/08/26 08:04:36 cisc Exp $

#pragma once

#include "types.h"

class ASPI
{
public:
	enum Direction
	{
		none = 0, in = 0x08, out = 0x10,
	};
public:
	ASPI();
	~ASPI();

	uint GetNHostAdapters() { return nhostadapters; }
	bool PrintHAInquiry(uint ha);
	bool InquiryAdapter(uint ha, uint* maxid, uint* maxxfer);
	int GetDeviceType(uint ha, uint id, uint lun);
	int ExecuteSCSICommand(uint ha, uint id, uint lun, void* cdb, uint cdblen, uint dir = 0, void* data = 0, uint datalen = 0);

private:
	uint32 SendCommand(void*);
	uint32 SendCommandAndWait(void*);
	bool ConnectAPI();
	void AbortService(uint, void*);

	uint32 (__cdecl * psac)(void*);
	uint32 (__cdecl * pgasi)();
	uint nhostadapters;
	HANDLE hevent;
	HMODULE hmod;
};

struct LONGBE
{
	uchar image[4];
	LONGBE() {}
	LONGBE(uint32 a)
	{
		image[0] = uchar(a >> 24);	image[1] = uchar(a >> 16);
		image[2] = uchar(a >> 8);	image[3] = uchar(a);
	}
	operator uint32()
	{
		return image[3] + image[2] * 0x100ul 
				+ image[1] * 0x10000ul + image[0] * 0x1000000ul;
	}
};

struct TRIBE
{
	uchar image[3];
	TRIBE() {}
	TRIBE(uint a)
	{
		image[0] = uchar(a >> 16); image[1] = uchar(a >> 8);
		image[2] = uchar(a);
	}
	operator uint()
	{
		return image[2] + image[1] * 0x100 + image[0] * 0x10000;
	}
};


struct WORDBE
{
	uchar image[2];
	WORDBE() {}
	WORDBE(uint a)
	{
		image[0] = uchar(a >> 8);	image[1] = uchar(a);
	}
	operator uint()
	{
		return image[1] + image[0] * 0x100;
	}
};

