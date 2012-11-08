// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1999, 2000.
// ---------------------------------------------------------------------------
//	DirectSound based driver - another version
// ---------------------------------------------------------------------------
//	$Id: soundds2.h,v 1.2 2002/05/31 09:45:21 cisc Exp $

#pragma once

#include "sounddrv.h"

// ---------------------------------------------------------------------------

namespace WinSoundDriver
{

class DriverDS2 : public Driver
{
private:
	enum
	{
		nblocks = 4,		// 2 à»è„
	};

public:
	DriverDS2();
	~DriverDS2();

	bool Init(SoundSource* sb, HWND hwnd, uint rate, uint ch, uint buflen);
	bool Cleanup();

private:
	static uint WINAPI ThreadEntry(LPVOID arg);
	void Send();

	LPDIRECTSOUND lpds;
	LPDIRECTSOUNDBUFFER lpdsb_primary;
	LPDIRECTSOUNDBUFFER lpdsb;
	LPDIRECTSOUNDNOTIFY lpnotify;
	
	uint buffer_length;
	HANDLE hthread;
	uint idthread;
	HANDLE hevent;
	uint nextwrite;
};

}

