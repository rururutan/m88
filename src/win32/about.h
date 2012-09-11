// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	About Dialog Box for M88
// ---------------------------------------------------------------------------
//	$Id: about.h,v 1.5 1999/12/28 11:14:05 cisc Exp $

#if !defined(WIN_ABOUT_H)
#define WIN_ABOUT_H

#include "types.h"
#include "instthnk.h"

// ---------------------------------------------------------------------------

class M88About  
{
public:
	M88About();
	~M88About() {}
	
	void Show(HINSTANCE, HWND);

private:
	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK DlgProcGate(M88About*, HWND, UINT, WPARAM, LPARAM);
	
	static const char abouttext[];
	InstanceThunk dlgproc;
	uint32 crc;
};

#endif // !defined(WIN_ABOUT_H)
