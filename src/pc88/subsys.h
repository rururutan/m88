// ---------------------------------------------------------------------------
//	PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: subsys.h,v 1.7 2000/09/08 15:04:14 cisc Exp $

#pragma once

#include "device.h"
#include "fdc.h"
#include "pio.h"

class MemoryManager;

namespace PC8801
{

class SubSystem : public Device
{
public:
	enum
	{
		reset = 0,  
		m_set0, m_set1, m_set2, m_setcw, 
		s_set0, s_set1, s_set2, s_setcw,
	};
	enum
	{
		intack = 0, 
		m_read0, m_read1, m_read2, s_read0, s_read1, s_read2
	};

public:
	SubSystem(const ID& id);
	~SubSystem();
	
	bool Init(MemoryManager* mmgr);
	const Descriptor* IFCALL GetDesc() const { return &descriptor; }
	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);

	uint8* GetRAM() { return ram; }
	uint8* GetROM() { return rom; }

	bool IsBusy();

	void IOCALL Reset(uint=0, uint=0);
	uint IOCALL IntAck(uint);
	
	void IOCALL M_Set0(uint, uint data);
	void IOCALL M_Set1(uint, uint data);
	void IOCALL M_Set2(uint, uint data);
	void IOCALL M_SetCW(uint, uint data);
	uint IOCALL M_Read0(uint);
	uint IOCALL M_Read1(uint);
	uint IOCALL M_Read2(uint);

	void IOCALL S_Set0(uint, uint data);
	void IOCALL S_Set1(uint, uint data);
	void IOCALL S_Set2(uint, uint data);
	void IOCALL S_SetCW(uint, uint data);
	uint IOCALL S_Read0(uint);
	uint IOCALL S_Read1(uint);
	uint IOCALL S_Read2(uint);

private:
	enum
	{
		ssrev = 1,
	};
	struct Status
	{
		uint rev;
		uint8 ps[3], cs;
		uint8 pm[3], cm;
		uint idlecount;
		uint8 ram[0x4000];
	};

	bool InitMemory();
	bool LoadROM();
	void PatchROM();

	MemoryManager* mm;
	int mid;
	uint8* rom;
	uint8* ram;
	uint8* dummy;
	PIO piom, pios;
	uint cw_m, cw_s;
	uint idlecount;

private:
	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};

} // namespace PC8801

