// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: loadmon.h,v 1.1 2001/02/21 11:58:54 cisc Exp $

#pragma once

#include "device.h"
#include "winmon.h"
#include "critsect.h"

//#define ENABLE_LOADMONITOR

// ---------------------------------------------------------------------------

#ifdef ENABLE_LOADMONITOR

class LoadMonitor : public WinMonitor
{
public:
	enum
	{
		presis = 10,
	};

	LoadMonitor();
	~LoadMonitor();

	bool Init(); 

	void ProcBegin(const char* name);
	void ProcEnd(const char* name);
	static LoadMonitor* GetInstance() { return instance; }

private:
	struct State
	{
		int	total[presis];				// ó›åv
		DWORD timeentered;		// äJénéûçè
	};

	typedef map<string, State> States;

	void UpdateText();
	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	void DrawMain(HDC, bool);

	States states;
	int base;
	int tidx;
	int tprv;

	CriticalSection cs;
	static LoadMonitor* instance;
};

inline void LOADBEGIN(const char* key)
{
	LoadMonitor* lm = LoadMonitor::GetInstance();
	if (lm) lm->ProcBegin(key);
}

inline void LOADEND(const char* key)
{
	LoadMonitor* lm = LoadMonitor::GetInstance();
	if (lm) lm->ProcEnd(key);
}

#else

class LoadMonitor
{
public:
	LoadMonitor() {}
	~LoadMonitor() {}

	bool Show(HINSTANCE, HWND, bool) { return false; }
	bool IsOpen() { return false; }
	bool Init() { return false; }
};


#define LOADBEGIN(a)
#define LOADEND(a)

#endif

