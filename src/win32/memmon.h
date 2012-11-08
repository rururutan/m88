// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: memmon.h,v 1.9 2003/05/19 02:33:56 cisc Exp $

#pragma once

#include "device.h"
#include "mvmon.h"
#include "pc88/memview.h"
#include "wincore.h"

// ---------------------------------------------------------------------------

class PC88;

namespace PC8801
{

class MemoryMonitor : public MemViewMonitor
{
public:
	MemoryMonitor();
	~MemoryMonitor();

	bool Init(WinCore*); 

private:
	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	BOOL EDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK EDlgProcGate(HWND, UINT, WPARAM, LPARAM);

	static uint MEMCALL MemRead(void* p, uint a);
	static void MEMCALL MemWrite(void* p, uint a, uint d);

	void Start();
	void Stop();

	void UpdateText();
	bool SaveImage();

	void SetWatch(uint, uint);

	void ExecCommand();
	void Search(uint key, int bytes);

	void SetBank();

	WinCore* core;
	IMemoryManager* mm;
	IGetMemoryBank* gmb;
	int mid;
	uint time;

	int prevaddr, prevlines;

	int watchflag;

	HWND hwndstatus;

	int editaddr;

	char line[0x100];
	uint8 stat[0x10000];
	uint access[0x10000];

	static COLORREF col[0x100];
};

}

