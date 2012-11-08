// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: main.cpp,v 1.9 2001/02/21 11:58:55 cisc Exp $

#include "headers.h"
#include "ui.h"
#include "file.h"

// ---------------------------------------------------------------------------

char m88dir[MAX_PATH];
char m88ini[MAX_PATH];

// ---------------------------------------------------------------------------
//	InitPathInfo
//
static void InitPathInfo()
{
	char buf[MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	GetModuleFileName(0, buf, MAX_PATH);
	_splitpath(buf, drive, dir, fname, ext); 
	sprintf(m88ini, "%s%s%s.ini", drive, dir, fname);
	sprintf(m88dir, "%s%s", drive, dir);
}

// ---------------------------------------------------------------------------
//	WinMain
//
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR cmdline, int nwinmode)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	if (FAILED(CoInitialize(NULL)))
		return -1;

	InitPathInfo();
	InitCommonControls();
		
	int r = -1;

	WinUI* ui = new WinUI(hinst);
	if (ui && ui->InitWindow(nwinmode))
	{
		r = ui->Main(cmdline);
	}
	delete ui;

	CoUninitialize();
	return r;
}

