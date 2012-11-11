// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//	$Id: mouse.h,v 1.1 2002/04/07 05:40:10 cisc Exp $

#pragma once

#include "device.h"
#include "if/ifui.h"

class PC88;

namespace PC8801
{

class Config;

class Mouse : public Device  
{
public:
	enum
	{
		strobe=0, vsync,
		getmove=0, getbutton,
	};
public:
	Mouse(const ID& id);
	~Mouse();

	bool Init(PC88* pc);
	bool Connect(IUnk* ui);

	const Descriptor* IFCALL GetDesc() const { return &descriptor; } 
	
	uint IOCALL GetMove(uint);
	uint IOCALL GetButton(uint);
	void IOCALL Strobe(uint, uint data);
	void IOCALL VSync(uint, uint);
	
	void ApplyConfig(const Config* config);

private:
	PC88* pc;
	POINT move;
	uint8 port40;
	bool joymode;
	int phase;
	int32 triggertime;
	int sensibility;	
	int data;

	IMouseUI* ui;

private:
	static const Descriptor descriptor;
	static const InFuncPtr indef[];
	static const OutFuncPtr outdef[];
};

}

