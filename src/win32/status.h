// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: status.h,v 1.6 2002/04/07 05:40:10 cisc Exp $

#pragma once

#include "types.h"
#include "critsect.h"

class StatusDisplay
{
public:
	StatusDisplay();
	~StatusDisplay();

	bool Init(HWND hwndparent);
	void Cleanup();

	bool Enable(bool sfs=false);
	bool Disable();
	int GetHeight() { return height; }
	void DrawItem(DRAWITEMSTRUCT* dis);
	void FDAccess(uint dr, bool hd, bool active);
	void UpdateDisplay();
	void WaitSubSys() { litstat[2] = 9; }

	bool Show(int priority, int duration, char* msg, ...);
	void Update();
	UINT_PTR GetTimerID() { return timerid; }

	HWND GetHWnd() { return hwnd; }

private:
	struct List
	{
		List* next;
		int priority;
		int duration;
		char msg[127];
		bool clear;
	};
	struct Border
	{
		int horizontal;
		int vertical;
		int split;
	};

	void Clean();
	
	HWND hwnd;
	HWND hwndparent;
	List* list;
	UINT_PTR timerid;
	CriticalSection cs;
	Border border;
	int height;
	int litstat[3];
	int litcurrent[3];
	bool showfdstat;
	bool updatemessage;

	int currentduration;
	int currentpriority;

	char buf[128];
};

extern StatusDisplay statusdisplay;

