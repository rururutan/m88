// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: cdctrl.h,v 1.2 1999/10/10 01:39:00 cisc Exp $

#pragma once

#include "winbase.h"
#include "device.h"

class CDROM;

class CDControl
{
public:
	enum
	{
		dummy = 0,
		readtoc, playtrack, stop, checkdisk, playrev,
		playaudio, readsubcodeq, pause, read1, read2,
		ncmds
	};

	struct Time
	{
		uchar track;
		uchar min;
		uchar sec;
		uchar frame;
	};

	typedef void (Device::*DONEFUNC)(int result);

public:
	CDControl();
	~CDControl();

	bool Init(CDROM* cd, Device* dev, DONEFUNC func);
	bool SendCommand(uint cmd, uint arg1 = 0, uint arg2 = 0);
	uint GetTime();

private:	
	void ExecCommand(uint cmd, uint arg1, uint arg2);
	void Cleanup();
	uint ThreadMain();
	static uint __stdcall ThreadEntry(LPVOID arg);
	
	HANDLE hthread;
	uint idthread;
	int vel;
	bool diskpresent;
	bool shouldterminate;

	Device* device;
	DONEFUNC donefunc;
	
	CDROM* cdrom;
};

