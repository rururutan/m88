// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: codemon.h,v 1.7 2003/05/19 02:33:56 cisc Exp $

#pragma once

#include "mvmon.h"
#include "device.h"
#include "pc88/memview.h"
#include "Z80diag.h"

// ---------------------------------------------------------------------------

class PC88;

namespace PC8801
{

class CodeMonitor : public MemViewMonitor
{
public:
	CodeMonitor();
	~CodeMonitor();

	bool Init(PC88*); 

private:
	void UpdateText();
	int VerticalScroll(int msg);
	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);

	bool Dump(FILE* fp, int from, int to);
	bool DumpImage();
	
	Z80Diag diag;
};

}

