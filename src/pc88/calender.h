// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  カレンダ時計(μPD1990) のエミュレーション
// ---------------------------------------------------------------------------
//	$Id: calender.h,v 1.3 1999/10/10 01:47:04 cisc Exp $

#pragma once

#include "device.h"

namespace PC8801
{

class Calender : public Device  
{
public:
	enum
	{
		reset = 0, out10, out40,
		in40 = 0,
	};
public:
	Calender(const ID& id);
	~Calender();
	bool Init() { return true; } 

	const Descriptor* IFCALL GetDesc() const { return &descriptor; }

	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);
	
	void IOCALL Out10(uint, uint data);
	void IOCALL Out40(uint, uint data);
	uint IOCALL In40(uint);
	void IOCALL Reset(uint=0, uint=0);

private:
	enum
	{
		ssrev = 1
	};
	struct Status
	{
		uint8 rev;
		bool dataoutmode;
		bool hold;
		uint8 datain;
		uint8 strobe;
		uint8 cmd, scmd, pcmd;
		uint8 reg[6];
		time_t t;
	};

	void ShiftData();
	void Command();

	void SetTime();
	void GetTime();

	time_t diff;

	bool dataoutmode;
	bool hold;
	uint8 datain;
	uint8 strobe;
	uint8 cmd, scmd, pcmd;
	uint8 reg[6];
	
	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};

}

