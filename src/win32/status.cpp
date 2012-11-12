// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: status.cpp,v 1.8 2002/04/07 05:40:10 cisc Exp $

#include "headers.h"
#include "status.h"

//#define LOGNAME "status"
#include "diag.h"

StatusDisplay statusdisplay;

// ---------------------------------------------------------------------------
//	Constructor/Destructor
//
StatusDisplay::StatusDisplay()
{
	hwnd = 0;
	hwndparent = 0;
	list = 0;
	timerid = 0;
	height = 0;
	litstat[0] = litstat[1] = litstat[2] = 0;
	litcurrent[0] = litcurrent[1] = litcurrent[2] = 0;
	updatemessage = 0;
	showfdstat = false;
	currentduration = 0;
	currentpriority = 10000;
}
	
StatusDisplay::~StatusDisplay()
{
	Cleanup();
	while (list)
	{
		List* next = list->next;
		delete list;
		list = next;
	}
}

bool StatusDisplay::Init(HWND hwndp)
{
	hwndparent = hwndp;
	return true;
}

bool StatusDisplay::Enable(bool showfd)
{
	if (!hwnd)
	{
		hwnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE, 0, hwndparent, 1);
		
		if (!hwnd)
			return false;
	}
	{
		showfdstat = showfd;

		SendMessage(hwnd, SB_GETBORDERS, 0, (LPARAM) &border);
		
		RECT rect;
		GetWindowRect(hwndparent, &rect);
		int widths[2];
		widths[0] = (rect.right-rect.left-border.vertical) - 1*((showfd ? 80 : 64)+border.split);
		widths[1] = -1;
		SendMessage(hwnd, SB_SETPARTS, 2, (LPARAM) widths);
		InvalidateRect(hwndparent, 0, false);
		
		GetWindowRect(hwnd, &rect);
		height = rect.bottom - rect.top;
	
		PostMessage(hwnd,  SB_SETTEXT, SBT_OWNERDRAW | 1, 0);
	}
	return true;
}

bool StatusDisplay::Disable()
{
	if (hwnd)
	{
		DestroyWindow(hwnd);
		hwnd = 0;
		height = 0;
	}
	return true;
}

void StatusDisplay::Cleanup()
{
	Disable();
	if (timerid) {
		KillTimer(hwndparent, timerid);
		timerid = 0;
	}
}

// ---------------------------------------------------------------------------
//	DrawItem
//
void StatusDisplay::DrawItem(DRAWITEMSTRUCT* dis)
{
	switch (dis->itemID)
	{
	case 0:
		{
			SetBkColor(dis->hDC, GetSysColor(COLOR_3DFACE));
			SetTextColor(dis->hDC, GetSysColor(COLOR_MENUTEXT));
			char* text = reinterpret_cast<char*>(dis->itemData);
			TextOut(dis->hDC, dis->rcItem.left, dis->rcItem.top, text, strlen(text));
			break;
		}
	case 1:
		{
			const DWORD color[] =
			{
				RGB(64, 0, 0), RGB(255, 0, 0), RGB(0, 64, 0), RGB(0, 255, 0),
				RGB(0, 32, 64), RGB(0, 128, 255),
			};

			HBRUSH hbrush1, hbrush2, hbrush3, hbrushold;
			hbrush1 = CreateSolidBrush(color[litcurrent[1] & 3]);
			hbrush2 = CreateSolidBrush(color[litcurrent[0] & 3]);
			hbrush3 = CreateSolidBrush(color[4 | (litcurrent[2] & 1)]);
			hbrushold = (HBRUSH) SelectObject(dis->hDC, hbrush1);
			Rectangle(dis->hDC, dis->rcItem.left+6, height/2 - 4, dis->rcItem.left + 24, height/2 + 5);
			SelectObject(dis->hDC, hbrush2);
			Rectangle(dis->hDC, dis->rcItem.left+32, height/2 - 4, dis->rcItem.left + 50, height/2 + 5);
			if (showfdstat)
			{
				SelectObject(dis->hDC, hbrush3);
				Rectangle(dis->hDC, dis->rcItem.left+58, height/2 - 4, dis->rcItem.left + 68, height/2 + 5);
			}
			SelectObject(dis->hDC, hbrushold);
			DeleteObject(hbrush1);
			DeleteObject(hbrush2);
			DeleteObject(hbrush3);
			break;
		}
	}	
}

// ---------------------------------------------------------------------------
//	メッセージ追加
//
bool StatusDisplay::Show(int priority, int duration, char* msg, ...)
{
	CriticalSection::Lock lock(cs);
	
	if (currentpriority < priority)
		if (!duration || (GetTickCount() + duration - currentduration) < 0)
			return true;

	List* entry = new List;
	if (!entry)
		return false;
	memset(entry, 0, sizeof(List));

	va_list marker;
	va_start(marker, msg);
	int tl = wvsprintf(entry->msg, msg, marker);
	assert(tl < 128);
	va_end(marker);

	entry->duration = GetTickCount() + duration;
	entry->priority = priority;
	entry->next = list;
	entry->clear = duration != 0;
	list = entry;

	LOG3("reg : [%s] p:%5d d:%8d\n", entry->msg, entry->priority, entry->duration);
	updatemessage = true;
	return true;
}

// ---------------------------------------------------------------------------
//	更新
//
void StatusDisplay::Update()
{
	updatemessage = false;
	if (hwnd)
	{
		CriticalSection::Lock lock(cs);
		// find highest priority (0 == highest)
		int pc = 10000;
		List* entry = 0;
		int c = GetTickCount();
		for (List* l = list; l; l=l->next)
		{
//			LOG3("\t\t[%s] p:%5d d:%8d\n", l->msg, l->priority, l->duration);
			if ((l->priority < pc) && ((!l->clear) || (l->duration - c) > 0))
				entry = l, pc = l->priority;
		}
		if (entry)
		{
			LOG3("show: [%s] p:%5d d:%8d\n", entry->msg, entry->priority, entry->duration);
			memcpy(buf, entry->msg, 128);
			PostMessage(hwnd,  SB_SETTEXT, SBT_OWNERDRAW | 0, (LPARAM) buf);
			
			if (entry->clear)
			{
				timerid = ::SetTimer(hwndparent, 8, entry->duration - c, 0);
				currentpriority = entry->priority;
				currentduration = entry->duration;
			}
			else
			{
				currentpriority = 10000;
				if (timerid)
				{
					KillTimer(hwndparent, timerid);
					timerid = 0;
				}
			}
			Clean();
		}
		else
		{
			LOG0("clear\n");
			PostMessage(hwnd, SB_SETTEXT, 0, (LPARAM) " ");
			currentpriority = 10000;
			if (timerid)
			{
				KillTimer(hwndparent, timerid);
				timerid = 0;
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	必要ないエントリの削除
//
void StatusDisplay::Clean()
{
	List** prev = &list;
	int c = GetTickCount();
	while (*prev)
	{
		if ((((*prev)->duration - c) < 0) || !(*prev)->clear)
		{
			List* de = *prev;
			*prev = de->next;
			delete de;
		}
		else
			prev = &(*prev)->next;
	}
}

// ---------------------------------------------------------------------------
//
//
void StatusDisplay::FDAccess(uint dr, bool hd, bool active)
{
	dr &= 1;
	if (!(litstat[dr] & 4))
	{
		litstat[dr] = (hd ? 0x22 : 0) | (active ? 0x11 : 0) | 4;
	}
	else
	{
		litstat[dr] = (litstat[dr] & 0x0f) | (hd ? 0x20 : 0) | (active ? 0x10 : 0) | 9;
	}
}

void StatusDisplay::UpdateDisplay()
{
	bool update = false;
	for (int d=0; d<3; d++)
	{
		if ((litstat[d] ^ litcurrent[d]) & 3)
		{
			litcurrent[d] = litstat[d];
			update = true;
		}
		if (litstat[d] & 8)
			litstat[d] >>= 4;
	}
	if (hwnd)
	{
		if (update)
			PostMessage(hwnd,  SB_SETTEXT, SBT_OWNERDRAW | 1, 0);
		if (updatemessage)
			Update();
	}
}

// 
