// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: WinJoy.cpp,v 1.6 2003/04/22 13:16:35 cisc Exp $

// #define	DIRECTINPUT_VERSION		0x0300

#include "headers.h"
#include "WinJoy.h"
#include "pc88/config.h"
#include "status.h"

using namespace PC8801;


// ---------------------------------------------------------------------------
//	構築/消滅
//
WinPadIF::WinPadIF()
{
	enabled = false;
}

WinPadIF::~WinPadIF()
{

}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinPadIF::Init()
{
	enabled = false;
	if (!joyGetNumDevs())
	{
		statusdisplay.Show(70, 3000, "ジョイスティック API を使用できません");
		return false;
	}

	JOYINFO joyinfo;
	if (joyGetPos(JOYSTICKID1, &joyinfo) == JOYERR_UNPLUGGED)
	{
		statusdisplay.Show(70, 3000, "ジョイスティックが接続されていません");
		return false;
	}
	enabled = true;
	return true;
}

// ---------------------------------------------------------------------------
//	ジョイスティックの状態を更新
//
void IFCALL WinPadIF::GetState(PadState* d)
{
	const int threshold = 16384;
	JOYINFO joyinfo;
	if (enabled && joyGetPos(JOYSTICKID1, &joyinfo) == JOYERR_NOERROR)
	{
		d->direction = 
				   (joyinfo.wYpos < (32768-threshold) ? 1 : 0)	// U
				 | (joyinfo.wYpos > (32768+threshold) ? 2 : 0)	// D
				 | (joyinfo.wXpos < (32768-threshold) ? 4 : 0)	// L
				 | (joyinfo.wXpos > (32768+threshold) ? 8 : 0);	// R
		d->button = (uint8) joyinfo.wButtons;
	}
	else
		d->direction = d->button = 0;
}

