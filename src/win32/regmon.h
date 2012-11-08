// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: regmon.h,v 1.1 2000/11/02 12:43:51 cisc Exp $

#pragma once

#include "device.h"
#include "winmon.h"
#include "pc88/pc88.h"


class PC88;

// ---------------------------------------------------------------------------

class Z80RegMonitor : public WinMonitor
{
public:
	Z80RegMonitor();
	~Z80RegMonitor();

	bool Init(PC88* pc); 

private:
	void UpdateText();
	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	void DrawMain(HDC, bool);

	PC88* pc;
};

