// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: soundmon.h,v 1.11 2003/06/12 13:14:37 cisc Exp $

#pragma once

#include "device.h"
#include "winmon.h"
#include "soundsrc.h"

// ---------------------------------------------------------------------------

namespace PC8801
{
	class OPNIF;
};


class OPNMonitor : public WinMonitor, public ISoundSource
{
public:
	OPNMonitor();
	~OPNMonitor();

	bool Init(PC8801::OPNIF* opn, ISoundControl* soundcontrol); 

	bool IFCALL SetRate(uint rate) { return true; }
	void IFCALL Mix(int32* s, int length);

private:
	enum
	{
		bufsize = 2048,
	};

	void UpdateText();
	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	void DrawMain(HDC, bool);

	bool IFCALL Connect(ISoundControl* sc);
	
	PC8801::OPNIF* opn;
	const uint8* regs;

	ISoundControl* soundcontrol;

	void Start();
	void Stop();
	
	uint mask;
	uint read;
	uint write;
	int dim;
	int dimvector;
	int width;
	int buf[2][bufsize];
};

