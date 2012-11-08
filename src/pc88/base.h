// ---------------------------------------------------------------------------
//	PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: base.h,v 1.10 2000/06/26 14:05:30 cisc Exp $

#pragma once

#include "schedule.h"
#include "device.h"
#include "Z80C.h"

class PC88;
class TapeManager;

namespace PC8801
{

class Config;

class Base : public Device
{
public:
	enum IDOut
	{
		reset=0, vrtc
	};
	enum IDIn
	{
		in30 = 0, in31, in40, in6e
	};

public:
	Base(const ID& id);
	~Base();
	
	bool Init(PC88* pc88);
	const Descriptor* IFCALL GetDesc() const { return &descriptor; }

	void SetSwitch(const Config* cfg);
	uint GetBasicMode() { return bmode; }
	void IOCALL Reset(uint=0, uint=0);
	void SetFDBoot(bool autoboot_) { autoboot = autoboot_; }

	void IOCALL RTC(uint=0);
	void IOCALL VRTC(uint, uint en);
	
	uint IOCALL In30(uint);
	uint IOCALL In31(uint);
	uint IOCALL In40(uint);
	uint IOCALL In6e(uint);
		
private:
	PC88* pc;

	int dipsw;
	int flags;
	int clock;
	int bmode;
	
	uint8 port40;
	uint8 sw30, sw31, sw6e;
	bool autoboot;
	bool fv15k;

	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};

}

