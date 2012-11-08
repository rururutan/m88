// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: winmouse.h,v 1.5 2002/04/07 05:40:11 cisc Exp $

#pragma once

#include "Device.h"
#include "if/ifui.h"

class WinUI;

class WinMouseUI : public IMouseUI
{
public:
	WinMouseUI();
	~WinMouseUI();

	bool Init(WinUI* ui);

	long IFCALL QueryInterface(REFIID, void**);
	ulong IFCALL AddRef();
	ulong IFCALL Release();

	bool IFCALL Enable(bool en);
	bool IFCALL GetMovement(POINT*);
	uint IFCALL GetButton();

private:
	POINT GetWindowCenter();
	
	WinUI* ui;
	
	POINT move;
	int32 activetime;
	bool enable;
	int orgmouseparams[3];

	ulong refcount;
};

