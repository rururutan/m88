// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	DirectSound based driver
// ---------------------------------------------------------------------------
//	$Id: soundds.h,v 1.2 2002/05/31 09:45:21 cisc Exp $

#pragma once

#include "sounddrv.h"

// ---------------------------------------------------------------------------

namespace WinSoundDriver
{

class DriverDS : public Driver
{
	static const uint num_blocks;
	static const uint timer_resolution;

public:
	DriverDS();
	~DriverDS();

	bool Init(SoundSource* sb, HWND hwnd, uint rate, uint ch, uint buflen);
	bool Cleanup();

private:
	static void CALLBACK TimeProc(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
	void Send();

	LPDIRECTSOUND lpds;
	LPDIRECTSOUNDBUFFER lpdsb_primary;
	LPDIRECTSOUNDBUFFER lpdsb;
	UINT timerid;
	LONG sending;
	
	uint nextwrite;
	uint buffer_length;
};

}

