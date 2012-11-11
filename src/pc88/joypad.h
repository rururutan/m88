// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: joypad.h,v 1.3 2003/05/19 01:10:31 cisc Exp $

#pragma once

#include "device.h"
#include "if/ifui.h"

namespace PC8801
{

class JoyPad : public Device  
{
public:
	enum
	{
		vsync = 0,
		getdir = 0, getbutton = 1,
	};
	enum ButtonMode
	{
		NORMAL, SWAPPED, DISABLED
	};

public:
	JoyPad();
	~JoyPad();

	bool Connect(IPadInput* ui);
	const Descriptor* IFCALL GetDesc() const { return &descriptor; } 
	
	void IOCALL Reset() {}
	uint IOCALL GetDirection(uint port);
	uint IOCALL GetButton(uint port);
	void IOCALL VSync(uint=0, uint=0);
	void SetButtonMode(ButtonMode mode);

private:
	void Update();

	IPadInput* ui;
	bool paravalid;
	uint8 data[2];

	uint8 button1;
	uint8 button2;
	uint directionmask;

private:
	static const Descriptor descriptor;
	static const InFuncPtr indef[];
	static const OutFuncPtr outdef[];
};

}

