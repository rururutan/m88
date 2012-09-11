// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//	Mouse Device
//	based on mouse code written by norimy.
// ---------------------------------------------------------------------------
//	$Id: mouse.cpp,v 1.1 2002/04/07 05:40:10 cisc Exp $

#include "headers.h"
#include "config.h"
#include "pc88.h"
#include "mouse.h"
#include "if/ifguid.h"

//#define LOGNAME "mouse"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築
//
Mouse::Mouse(const ID& id)
: Device(id), ui(0)
{
}

Mouse::~Mouse()
{
	RELEASE(ui);
}

// ---------------------------------------------------------------------------
//	初期化
//
bool Mouse::Init(PC88* pc88)
{
	pc = pc88;
	return true;
}

bool Mouse::Connect(IUnk* unk)
{
	RELEASE(ui);
	if (unk)
		unk->QueryInterface(ChIID_MouseUI, (void**) &ui);
	return !!ui;
}

// ---------------------------------------------------------------------------
//	入力
//
uint IOCALL Mouse::GetMove(uint)
{
	Log("%c", 'w' + phase);
	if (joymode)
	{
		if (data == -1)
		{
			data = 0xff;
			if (ui)
			{
				ui->GetMovement(&move);
				if (move.x >=  sensibility) data &= ~4;
				if (move.x <= -sensibility) data &= ~8;
				if (move.y >=  sensibility) data &= ~1;
				if (move.y <= -sensibility) data &= ~2;
			}
		}
		return data;
	}
	else
	{
		switch (phase & 3)
		{
		case 1:	// x
			return (move.x >> 4) | 0xf0;

		case 2:	// y
			return (move.x     ) | 0xf0;

		case 3:	// z
			return (move.y >> 4) | 0xf0;

		case 0:	// u
			return (move.y     ) | 0xf0;
		}
		return 0xff;
	}
}

uint IOCALL Mouse::GetButton(uint)
{
	return ui ? (~ui->GetButton()) | 0xfc : 0xff;
}


// ---------------------------------------------------------------------------
//	ストローブ信号
//
void IOCALL Mouse::Strobe(uint, uint data)
{
	data &= 0x40;
	if (port40 ^ data)
	{
		port40 = data;
		
		if (phase <= 0 || int(pc->GetTime() - triggertime) > 18*4)
		{
			if (data)
			{
				triggertime = pc->GetTime();
				if (ui)
					ui->GetMovement(&move);
				else
					move.x = move.y = 0;
				phase = 1;
				Log("\nStrobe (%4d, %4d, %d): ", move.x, move.y, triggertime);
			}
			return;
		}
		Log("[%d]", pc->GetTime() - triggertime);

		phase = (phase + 1) & 3;
	}
}


// ---------------------------------------------------------------------------
//
//
void IOCALL Mouse::VSync(uint, uint)
{
	data = -1;
}

// ---------------------------------------------------------------------------
//	コンフィギュレーション更新
//
void Mouse::ApplyConfig(const Config* config)
{
	joymode = (config->flags & Config::mousejoymode) != 0;
	sensibility = config->mousesensibility;
}


// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor Mouse::descriptor = { indef, outdef };

const Device::OutFuncPtr Mouse::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &Strobe),
	STATIC_CAST(Device::OutFuncPtr, &VSync),
};

const Device::InFuncPtr Mouse::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, &GetMove),
	STATIC_CAST(Device::InFuncPtr, &GetButton),
};
