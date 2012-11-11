// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: beep.h,v 1.2 1999/10/10 01:47:04 cisc Exp $

#pragma once

#include "device.h"

// ---------------------------------------------------------------------------

class PC88;

namespace PC8801
{
class Sound;
class Config;
class OPNIF;

// ---------------------------------------------------------------------------
//
//
class Beep : public Device, public ISoundSource
{
public:
	enum IDFunc
	{
		out40=0,
	};

public:
	Beep(const ID& id);
	~Beep();
	
	bool Init();
	void Cleanup();	
	void EnableSING(bool s) { p40mask = s ? 0xa0 : 0x20; port40 &= p40mask; }
	
	bool IFCALL Connect(ISoundControl* sc);
	bool IFCALL SetRate(uint rate);
	void IFCALL Mix(int32*, int);
		
	const Descriptor* IFCALL GetDesc() const { return &descriptor; }
	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);
	
	void IOCALL Out40(uint, uint data);

private:
	enum { ssrev = 1, };
	struct Status
	{
		uint8 rev;
		uint8 port40;
		uint32 prevtime;
	};
	
	ISoundControl* soundcontrol;
	int bslice;
	int pslice;
	int bcount;
	int bperiod;

	uint port40;
	uint p40mask;

	static const Descriptor descriptor;
	static const OutFuncPtr outdef[];
};

}

