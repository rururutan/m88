// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: loadmon.cpp,v 1.1 2001/02/21 11:58:54 cisc Exp $

#include "headers.h"
#include <algorithm>
#include <numeric>
#include "resource.h"
#include "loadmon.h"
#include "misc.h"

#ifdef ENABLE_LOADMONITOR

LoadMonitor* LoadMonitor::instance = 0;

// ---------------------------------------------------------------------------
//	構築/消滅
//
LoadMonitor::LoadMonitor()
{
}

LoadMonitor::~LoadMonitor()
{
	CriticalSection::Lock lock(cs);
	instance = 0;
}

bool LoadMonitor::Init()
{
	if (!WinMonitor::Init(MAKEINTRESOURCE(IDD_REGMON)))
		return false;
	
	SetUpdateTimer(1000 / presis);
	SetFont(GetHWnd(), 16);

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	base = freq.LowPart / 1000000;		// 1us 単位
	
	instance = this;
	tidx = 0;
	return true;
}

void LoadMonitor::DrawMain(HDC hdc, bool)
{
	RECT rect;
	GetClientRect(GetHWnd(), &rect);
	
	HBRUSH hbr = CreateSolidBrush(0x113300);
	hbr = (HBRUSH) SelectObject(hdc, hbr);
	PatBlt(hdc, rect.left, rect.top, rect.right, rect.bottom, PATCOPY);
	DeleteObject(SelectObject(hdc, hbr));

	SetTextColor(hdc, 0xffffff);
	WinMonitor::DrawMain(hdc, true);
}

// ---------------------------------------------------------------------------
//	ダイアログ処理
//
BOOL LoadMonitor::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	return WinMonitor::DlgProc(hdlg, msg, wp, lp);
}

// ---------------------------------------------------------------------------
//	状態を表示
//
void LoadMonitor::UpdateText()
{
	CriticalSection::Lock lock(cs);

	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	int tint = (t.LowPart - tprv) / base;
	tprv = t.LowPart;

	int c = 256000000 / (presis * (tint ? tint : 1));

	int nidx = (tidx + 1) % presis;
	for (States::iterator i = states.begin(); i != states.end(); ++i)
	{
		const int p = presis/8;
		int t;
		int buf[presis];
		
		i->second.total[tidx] = i->second.total[tidx] * c / 256;
		
		copy(i->second.total, i->second.total+presis, buf);
		sort(buf, buf+presis);
		t = accumulate(buf+p, buf+presis-p, 0) * presis /(presis-2*p);
		t = (t+4) / 10;
	
		Putf("%-12s %8d.%.2d\n", i->first.c_str(), t / 100, t % 100);
	
		i->second.total[nidx]= 0;
	}
	tidx = nidx;
}

// ---------------------------------------------------------------------------
//	記録開始
//
void LoadMonitor::ProcBegin(const char* name)
{
	CriticalSection::Lock lock(cs);

	string key(name);
	States::iterator i = states.find(key);
	
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	
	if (i != states.end())
	{
		i->second.timeentered = t.LowPart;
	}
	else
	{
		// 新しく登録
		State stat;
		memset(stat.total, 0, sizeof(stat.total));
		stat.timeentered = t.LowPart;
		states[key] = stat;
	}
}

// ---------------------------------------------------------------------------
//	記録終了
//
void LoadMonitor::ProcEnd(const char* name)
{
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);

	CriticalSection::Lock lock(cs);
	
	States::iterator i = states.find(string(name));

	if (i != states.end())
	{
		i->second.total[tidx] += (t.LowPart - i->second.timeentered) / base;
	}
}

#endif 