// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	NewDisk Dialog Box for M88
// ---------------------------------------------------------------------------
//	$Id: newdisk.h,v 1.4 1999/12/28 10:34:52 cisc Exp $

#if !defined(WIN32_NEWDISK_H)
#define WIN32_NEWDISK_H

#include "types.h"
#include "instthnk.h"

// ---------------------------------------------------------------------------

class WinNewDisk 
{
public:
	enum DiskType
	{
		MD2D=0, MD2DD, MD2HD
	};
public:
	WinNewDisk();
	~WinNewDisk() {}
	
	bool Show(HINSTANCE, HWND);
	const char* GetTitle() { return info.title; }
	DiskType GetType() { return info.type; }
	bool DoFormat() { return info.basicformat; }

	struct DiskInfo
	{
		char title[20];
		DiskType type;
		bool basicformat;
	};

private:
	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK DlgProcGate(WinNewDisk*, HWND, UINT, WPARAM, LPARAM);
	
	InstanceThunk dlgproc;
	DiskInfo info;
};

#endif // !defined(WIN32_NEWDISK_H)
