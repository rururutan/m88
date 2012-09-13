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
	GetModuleFileName(0, buf, MAX_PATH);
	
	LPTSTR filepart;
	GetFullPathName(buf, MAX_PATH, m88dir, &filepart);
	*filepart = 0;

	strcpy_s(m88ini, sizeof(m88ini), m88dir);
	strcat_s(m88ini, sizeof(m88ini), "M88.ini");
}

// ---------------------------------------------------------------------------
//	WinMain
//
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR cmdline, int nwinmode)
{
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

