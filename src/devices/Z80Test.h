// ----------------------------------------------------------------------------
//	M88 - PC-8801 emulator
//	Copyright (C) cisc 1998.
// ----------------------------------------------------------------------------
//	Z80 エンジン比較実行用クラス
//	$Id: Z80Test.h,v 1.2 1999/08/01 14:18:10 cisc Exp $

#ifndef Z80Test_h
#define Z80Test_h

#include "device.h"
#include "Z80.h"
#include "Z80C.h"
#include "Z80_x86.h"

// ----------------------------------------------------------------------------

class Z80Test : public Device
{
private:
	typedef Z80C CPURef;
	typedef Z80_x86 CPUTarget;

public:
	enum
	{
		reset = 0, irq, nmi,
	};

public:
	Z80Test(const ID& id);
	~Z80Test();

	bool Init(Bus* bus, int iack);
	MemoryBus::Page* GetPages() { return 0; }

	int Exec(int);
	static int ExecDual(Z80Test*, Z80Test*, int);
	void Stop(int);
	static void StopDual(int c) { currentcpu->Stop(c); }
	
	int GetCount() { return execcount + clockcount; }

	static int GetCCount() { return 0; }

	void Reset(uint=0, uint=0);
	void IRQ(uint, uint d);
	void NMI(uint, uint);
	void Wait(bool);
	const Descriptor* GetDesc() const { return &descriptor; }

private:
	uint codesize;
	CPURef    cpu1;
	CPUTarget cpu2;
	Z80Reg reg;
	static Z80Test* currentcpu;

	int execcount;
	int clockcount;

	uint pc;
	uint8 code[4];
	uint  readptr[8], writeptr[8], inptr, outptr;
	uint8 readdat[8], writedat[8], indat, outdat;
	uint readcount, writecount;
	uint readcountt, writecountt;
	int intr;

	FILE* fp;
	Bus* bus;
	Bus bus1;
	Bus bus2;

	void Test();
	void Error(const char*);

	uint Read8R(uint), Read8T(uint);
	void Write8R(uint, uint), Write8T(uint, uint);
	uint InR(uint), InT(uint);
	void OutR(uint, uint), OutT(uint, uint);

	static uint MEMCALL S_Read8R(void*, uint);
	static uint MEMCALL S_Read8T(void*, uint);
	static void MEMCALL S_Write8R(void*, uint, uint);
	static void MEMCALL S_Write8T(void*, uint, uint);

	static const Descriptor descriptor;
	static const OutFuncPtr outdef[];
};


#endif // Z80Test_h
