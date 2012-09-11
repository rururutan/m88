// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//	Mouse Interface for Win32
// ---------------------------------------------------------------------------
//	$Id: winmouse.cpp,v 1.9 2002/04/07 05:40:11 cisc Exp $

#include "headers.h"
#include "winmouse.h"
#include "ui.h"
#include "if/ifguid.h"
#include "messages.h"


WinMouseUI::WinMouseUI()
: enable(false), ui(0), refcount(0)
{
	activetime = 0;
}

WinMouseUI::~WinMouseUI()
{
	Enable(false);
}

bool WinMouseUI::Init(WinUI* _ui)
{
	ui = _ui;
	return true;
}

// ---------------------------------------------------------------------------
//	COM IUnk
//
long IFCALL WinMouseUI::QueryInterface(REFIID id, void** ppi)
{
	if (id == IID_IUnknown)				*ppi = this;
	else if (id == ChIID_MouseUI)		*ppi = this;
	else							  { *ppi = 0; return E_NOINTERFACE; }
	AddRef();
	return S_OK;
}

ulong IFCALL WinMouseUI::AddRef()
{
	return ++refcount;
}

ulong IFCALL WinMouseUI::Release()
{
	if (--refcount)
		return refcount;
	delete this;
	return 0;
}

// ---------------------------------------------------------------------------
//	マウスキャプチャリング開始・停止
//
bool WinMouseUI::Enable(bool en)
{
	if (enable != en)
	{
		enable = en;
		if (enable)
		{
			int mouseparams[3] = { 999, 999, 1 };
			
			if (!SystemParametersInfo(SPI_GETMOUSE, 0, orgmouseparams, 0))
				return false;
			if (!SystemParametersInfo(SPI_SETMOUSE, 0, mouseparams, 0))
				return false;

			ShowCursor(false);
			activetime = GetTickCount() + 500;
			
			SendMessage(ui->GetHWnd(), WM_M88_CLIPCURSOR, CLIPCURSOR_MOUSE, 0);
			return true;
		}
		else
		{
			SystemParametersInfo(SPI_SETMOUSE, 0, orgmouseparams, 0);
			ShowCursor(true);
			SendMessage(ui->GetHWnd(), WM_M88_CLIPCURSOR, -CLIPCURSOR_MOUSE, 0);
			return true;
		}
	}
	return true;
}

uint WinMouseUI::GetButton()
{
	return ui->GetMouseButton();
}

// ---------------------------------------------------------------------------
//	マウスの移動状況を取得
//
bool WinMouseUI::GetMovement(POINT* move)
{
	move->x = move->y = 0;
	
	if (enable && (!activetime || (int32(GetTickCount()) - activetime > 0)))
	{
		activetime = 0;
		POINT point;
		if (GetCursorPos(&point))
		{
			POINT c = GetWindowCenter();
			move->x = (c.x - point.x) / 2;
			move->y = (c.y - point.y) / 2;
			
			SetCursorPos(c.x, c.y);
			if (Abs(move->x) < 320 && Abs(move->y) < 200) 
			{
				move->x = Limit(move->x, 127, -127);
				move->y = Limit(move->y, 127, -127);
			}
			else 
				move->x = move->y = 0;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
//	ウィンドウ位置の取得
//
POINT WinMouseUI::GetWindowCenter()
{
	RECT rect;
	GetWindowRect(ui->GetHWnd(), &rect);
	
	POINT p;
	p.x = (rect.right + rect.left ) / 2;
	p.y = (rect.bottom + rect.top ) / 2;
	return p;
}

