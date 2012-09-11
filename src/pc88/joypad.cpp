// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//	$Id: joypad.cpp,v 1.3 2003/05/19 01:10:31 cisc Exp $

#include "headers.h"
#include "joypad.h"
#include "if/ifguid.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	\’zE”jŠü
//
JoyPad::JoyPad()
: Device(0), ui(0)
{
	SetButtonMode(NORMAL);
}

JoyPad::~JoyPad()
{
}

// ---------------------------------------------------------------------------
//
//
bool JoyPad::Connect(IPadInput* u)
{
	ui = u;

	return !!ui;
}

// ---------------------------------------------------------------------------
//	“ü—Í
//
uint IOCALL JoyPad::GetDirection(uint)
{
	if (!paravalid)
		Update();
	return data[0];
}

uint IOCALL JoyPad::GetButton(uint)
{
	if (!paravalid)
		Update();
	return data[1];
}

void JoyPad::Update()
{
	PadState ps;
	if (ui)
	{
		ui->GetState(&ps);
		data[0] = ~ps.direction | directionmask;
		data[1] = 
			  (ps.button & button1 ? 0 : 1) 
			| (ps.button & button2 ? 0 : 2)
			| 0xfc;
	}
	else
	{
		data[0] = data[1] = 0xff;
	}
	paravalid = true;
}

void JoyPad::SetButtonMode(ButtonMode mode)
{
	button1 = 1 | 4;
	button2 = 2 | 8;
	directionmask = 0xf0;

	switch (mode)
	{
	case SWAPPED:
		swap(button1, button2);
		break;
	case DISABLED:
		button1 = 0;
		button2 = 0;
		directionmask = 0xff;
		break;
	}
}

// ---------------------------------------------------------------------------
//	VSync ‚½‚¢‚Ý‚ñ‚®
//
void IOCALL JoyPad::VSync(uint, uint d)
{
	if (d)
		paravalid = false;
}
	
// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor JoyPad::descriptor = { indef, outdef };

const Device::OutFuncPtr JoyPad::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &VSync),
};

const Device::InFuncPtr JoyPad::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, &GetDirection),
	STATIC_CAST(Device::InFuncPtr, &GetButton),
};

