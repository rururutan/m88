// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: regmon.cpp,v 1.1 2000/11/02 12:43:51 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "regmon.h"
#include "misc.h"
#include "pc88/pc88.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築/消滅
//
Z80RegMonitor::Z80RegMonitor()
{
}

Z80RegMonitor::~Z80RegMonitor()
{
}

bool Z80RegMonitor::Init(PC88* _pc)
{
	if (!WinMonitor::Init(MAKEINTRESOURCE(IDD_REGMON)))
		return false;
	
	pc = _pc;
	SetUpdateTimer(50);
	
	return true;
}

void Z80RegMonitor::DrawMain(HDC hdc, bool)
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
BOOL Z80RegMonitor::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
/*	switch (msg)
	{
//	case WM_SIZE:
//		width = Min(LOWORD(lp) + 128, bufsize);
//		SetFont(hdlg, Limit(HIWORD(lp) / 11, 24, 8));
		break;
	}
*/	return WinMonitor::DlgProc(hdlg, msg, wp, lp);
}


// ---------------------------------------------------------------------------
//	状態を表示
//
void Z80RegMonitor::UpdateText()
{
	PC88::Z80* c1 = pc->GetCPU1();
	PC88::Z80* c2 = pc->GetCPU2();
	const Z80Reg& r1 = c1->GetReg();
	const Z80Reg& r2 = c2->GetReg();

	const uint m = 0xffff;

	Putf("PC:%.4x SP:%.4x   PC:%.4x SP:%.4x\n",	c1->GetPC() & m, r1.r.w.sp & m, c2->GetPC() & m, r2.r.w.sp & m);
	Putf("AF:%.4x BC:%.4x   AF:%.4x BC:%.4x\n",	r1.r.w.af & m,   r1.r.w.bc & m, r2.r.w.af & m,   r2.r.w.bc & m);
	Putf("HL:%.4x DE:%.4x   HL:%.4x DE:%.4x\n",	r1.r.w.hl & m,   r1.r.w.de & m, r2.r.w.hl & m,   r2.r.w.de & m);
	Putf("IX:%.4x IY:%.4x   IX:%.4x IY:%.4x\n",	r1.r.w.ix & m,   r1.r.w.iy & m, r2.r.w.ix & m,   r2.r.w.iy & m);
	Putf("AF'%.4x BC'%.4x   AF'%.4x BC'%.4x\n",	r1.r_af & m,     r1.r_bc & m,   r2.r_af & m,     r2.r_bc   & m);
	Putf("HL'%.4x DE'%.4x   HL'%.4x DE'%.4x\n",	r1.r_hl & m,     r1.r_de & m,   r2.r_hl & m,     r2.r_de & m);
	Putf("I :%.2x   IM:   %x   I :%.2x   IM:   %x\n",	r1.ireg, r1.intmode, r2.ireg, r2.intmode);
}
