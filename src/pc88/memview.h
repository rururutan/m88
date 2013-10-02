// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	メモリ監視インターフェース
// ----------------------------------------------------------------------------
//	$Id: memview.h,v 1.3 2001/02/21 11:57:57 cisc Exp $

#pragma once

#include "device.h"
#include "memory.h"
#include "subsys.h"
#include "pc88.h"

namespace PC8801
{
// ----------------------------------------------------------------------------
//	0	N88 N80 RAM ERAM			 SUB
//	60	N88 N80 RAM ERAM E0 E1 E2 E3 SUB
//	80  RAM
//	C0	RAM GV0 GV1 GV2
//	F0	RAM TV
//	
class MemoryViewer
{
public:
	enum Type
	{
		mainram = Memory::mRAM, 
		eram0	= Memory::mERAM, 
		eram1	= Memory::mERAM+1, 
		eram2	= Memory::mERAM+2, 
		eram3	= Memory::mERAM+3, 
		n88rom	= Memory::mN88, 
		nrom	= Memory::mN, 
		n88e0	= Memory::mN88E0, 
		n88e1	= Memory::mN88E1, 
		n88e2	= Memory::mN88E2, 
		n88e3	= Memory::mN88E3,
		gvram0	= Memory::mG0, 
		gvram1	= Memory::mG1, 
		gvram2	= Memory::mG2, 
		tvram	= Memory::mTV, 
		sub		= -1
	};
	
	MemoryViewer();
	~MemoryViewer();

	bool Init(PC88* pc);
	MemoryBus* GetBus() { return &bus; }
	void SelectBank(Type a0, Type a6, Type a8, Type ac, Type af);

	void StatClear();
	uint StatExec(uint pc);
	uint* StatExecBuf();

	uint GetCurrentBank(uint addr);

private:
	Memory* mem1;
	SubSystem* mem2;
	MemoryBus bus;
	
	PC88* pc;
	PC88::Z80* z80;

	Type bank[5];

protected:
#ifdef Z80C_STATISTICS
	Z80C::Statistics* stat;
#endif
};


inline uint MemoryViewer::GetCurrentBank(uint addr)
{
	static const int ref[16] = { 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4 };
	return bank[ref[addr >> 12]];
}

inline void MemoryViewer::StatClear()
{
#ifdef Z80C_STATISTICS
	if (stat)
		stat->Clear();
#endif
}

inline uint MemoryViewer::StatExec(uint pc)
{
#ifdef Z80C_STATISTICS
	if (stat)
		return stat->execute[pc];
#endif
	return 0;
}

inline uint* MemoryViewer::StatExecBuf()
{
#ifdef Z80C_STATISTICS
	if (stat)
		return stat->execute;
#endif
	return 0;
}

};

