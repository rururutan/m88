// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: beep.cpp,v 1.2 1999/10/10 01:47:04 cisc Exp $

#include "headers.h"
#include "pc88/beep.h"
#include "misc.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	生成・破棄
//
Beep::Beep(const ID& id)
: Device(id), soundcontrol(0)
{
}

Beep::~Beep()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化とか
//
bool Beep::Init()
{
	port40 = 0;
	p40mask = 0xa0;
	return true;
}

// ---------------------------------------------------------------------------
//	後片付け
//
void Beep::Cleanup()
{
	Connect(0);
}


bool IFCALL Beep::Connect(ISoundControl* control)
{
	if (soundcontrol) soundcontrol->Disconnect(this);
	soundcontrol = control;
	if (soundcontrol) soundcontrol->Connect(this);
	return true;
}

// ---------------------------------------------------------------------------
//	レート設定
//
bool Beep::SetRate(uint rate)
{
	pslice = 0;
	bslice = 0;
	bcount = 0;
	bperiod = int(2400.0 / rate * (1 << 14));
	return true;
}

// ---------------------------------------------------------------------------
//	ビープ音合成
//
//	0-2000
//	 0 -  4		1111
//	 5 -  9		0111
//	10 - 14		0011
//	15 - 19		0001
//
void IFCALL Beep::Mix(int32* dest, int nsamples)
{
	int i;
	int p = port40 & 0x80 ? 0 : 0x10000;
	int b = port40 & 0x20 ? 0 : 0x10000;
		
	uint ps = pslice, bs = bslice;
	pslice = bslice = 0;
	
	int sample = 0;
	for (i=0; i<8; i++)
	{
		if (ps < 500) p ^= 0x10000;
		if (bs < 500) b ^= 0x10000;

		sample += (b & bcount) | p;
		bcount += bperiod;
		ps -= 500;
		bs -= 500;
	}
	sample >>= 6;
	*dest++ += sample; *dest++ += sample;
	
	if (p | b)
	{
		for (i=nsamples-1; i>0; i--)
		{
			sample = 0;
			for (int j=0; j<8; j++)
			{
				sample += (b & bcount) | p;
				bcount += bperiod;
			}
			sample >>= 6;
			*dest++ += sample; *dest++ += sample;
		}
	}
}

// ---------------------------------------------------------------------------
//	BEEP Port への Out
//
void IOCALL Beep::Out40(uint, uint data)
{
	data &= p40mask;
	int i = data ^ port40;
	if (i & 0xa0)
	{
		if (soundcontrol)
		{
			soundcontrol->Update(this);

			int tdiff = soundcontrol->GetSubsampleTime(this);
			if (i & 0x80) pslice = tdiff;
			if (i & 0x20) bslice = tdiff;
		}
		port40 = data;
	}
}

// ---------------------------------------------------------------------------
//	状態保存
//
uint IFCALL Beep::GetStatusSize()
{
	return sizeof(Status);
}

bool IFCALL Beep::SaveStatus(uint8* s)
{
	Status* st = (Status*) s;
	st->rev = ssrev;
	st->port40 = port40;
	return true;
}

bool IFCALL Beep::LoadStatus(const uint8* s)
{
	const Status* st = (const Status*) s;
	if (st->rev != ssrev)
		return false;
	port40 = st->port40;
	return true;
}

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor Beep::descriptor = { 0, outdef };

const Device::OutFuncPtr Beep::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &Out40),
};

