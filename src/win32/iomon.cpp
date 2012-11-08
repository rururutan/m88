// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1999, 2000.
// ---------------------------------------------------------------------------
//	$Id: iomon.cpp,v 1.1 2001/02/21 11:58:54 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "pc88/pc88.h"
#include "pc88/ioview.h"
#include "iomon.h"
#include "misc.h"
#include "device_i.h"
#include "file.h"
#include "winvars.h"
#include "if/ifguid.h"

using namespace PC8801;

COLORREF IOMonitor::ctbl[0x100] = { 0 };

// ---------------------------------------------------------------------------
//	構築/消滅
//
IOMonitor::IOMonitor()
{
	if (!ctbl[0xff])
	{
		ctbl[0xff] = RGB(0x1f, 0x1f, 0x9f);
		for (int i=1; i<256; i++)
		{
			int r = 0x40 + static_cast<int>(0xbf * pow((i/256.), 8.0));
			int g = 0x20 + static_cast<int>(0xdf * pow((i/256.), 24.0));
			ctbl[0xff-i] = RGB(Min(r, 0xff), Min(g, 0xff), 0xff);
		}
	}
}

IOMonitor::~IOMonitor()
{
}

// ---------------------------------------------------------------------------
//	初期化
//
bool IOMonitor::Init(WinCore* _pc)
{
	pc = _pc;
	bank = true;
	if (!WinMonitor::Init(MAKEINTRESOURCE(IDD_IOVMON)))
		return false;
	SetLines(0x10);
	SetUpdateTimer(50);

	return true;
}


void IOMonitor::Start()
{
	pc->Lock();

	IIOBus* bus = (IIOBus*) pc->QueryIF(bank ? M88IID_IOBus1 : M88IID_IOBus2);
	if (bus)
		iov.Connect(bus);
	
	pc->Unlock();
}

void IOMonitor::Stop()
{
	pc->Lock();
	iov.Disconnect();
	pc->Unlock();
}


inline static void ToHex(char** p, uint d)
{
	static const char hex[] = "0123456789abcdef";

	*(*p)++ = hex[d >> 4];
	*(*p)++ = hex[d & 15];
}

// ---------------------------------------------------------------------------
//	テキスト更新
//
void IOMonitor::UpdateText()
{
	int a = GetLine() * 0x10;
	for (int y=0; y<GetHeight(); y++)
	{
		if (a < 0x100)
		{
			SetTxCol(0xffffff);
			Putf("%.2x: ", (a & 0xff));

			for (int x=0; x<16; x++)
			{
				uint d = iov.Read(a & 0xff);
				int c = d >> 24;
				
				SetTxCol(ctbl[c]);
				Putf("%.2x ", d & 0xff);
				a++;
			}
		}
		SetTxCol(0xffffff);
		Puts("\n");
	}
	iov.Dim();
}


// ---------------------------------------------------------------------------
//	ダイアログ処理
//
BOOL IOMonitor::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wp))
		{
		case IDM_IO_CLEAR:
			iov.Reset();
			break;

		case IDM_IO_MAIN:
		case IDM_IO_SUB:
			bank = LOWORD(wp) == IDM_IO_MAIN;
			
			if (IsOpen())
			{
				Stop();
				Start();
			}
			break;
		}
		break;
	}
	return WinMonitor::DlgProc(hdlg, msg, wp, lp);
}

