// ----------------------------------------------------------------------------
//	M88 - PC-88 emulator
//	Copyright (C) cisc 1998.
// ----------------------------------------------------------------------------
//	Z80 エミュレーションパッケージ比較実行用クラス
//	$Id: Z80Test.cpp,v 1.6 1999/08/14 14:45:06 cisc Exp $

#include "headers.h"
#include "Z80Test.h"
#include "device_i.h"

Z80Test* Z80Test::currentcpu=0;

// ----------------------------------------------------------------------------

Z80Test::Z80Test(const ID& id)
: Device(id), cpu1('cpur'), cpu2('cpur')
{
	char buf[] = "    .dmp";
	buf[0] = (id >> 24) & 0xff; buf[1] = (id >> 16) & 0xff;
	buf[2] = (id >>  8) & 0xff; buf[3] = (id      ) & 0xff;
	
	fp = fopen(buf, "w");
}

Z80Test::~Z80Test()
{
	if (fp)
		fclose(fp);
}

// ----------------------------------------------------------------------------

bool Z80Test::Init(Bus* bus_, int iack)
{
	bus = bus_;
	if (!bus1.Init(0x120, 0x10000 >> MemoryBus::pagebits, cpu1.GetPages()))
		return false;
	if (!bus2.Init(0x120, 0x10000 >> MemoryBus::pagebits, cpu2.GetPages()))
		return false;
	if (!cpu1.Init(&bus1, iack))
		return false;
	if (!cpu2.Init(&bus2, iack))
		return false;

	bus1.SetFuncs(0, 0x10000, this, S_Read8R, S_Write8R);
	bus2.SetFuncs(0, 0x10000, this, S_Read8T, S_Write8T);
	for (int p=0; p<0x120; p++)
	{
		bus1.ConnectIn (p, this, STATIC_CAST(InFuncPtr, &Z80Test::InR));
		bus1.ConnectOut(p, this, STATIC_CAST(OutFuncPtr, &Z80Test::OutR));
		bus2.ConnectIn (p, this, STATIC_CAST(InFuncPtr, &Z80Test::InT));
		bus2.ConnectOut(p, this, STATIC_CAST(OutFuncPtr, &Z80Test::OutT));
	}

	execcount = 0;

	return true;
}

// ----------------------------------------------------------------------------

int Z80Test::ExecDual(Z80Test* first, Z80Test* second, int nclocks)
{
	currentcpu = first;
	first->execcount += first->clockcount;
	second->execcount += second->clockcount;
	first->clockcount = -nclocks;
	second->clockcount = 0;

	first->readcount = first->writecount = first->readcountt = first->writecountt = first->codesize = 0;
	first->cpu1.TestIntr();	first->cpu2.TestIntr();
	second->readcount = second->writecount = second->readcountt = second->writecountt = second->codesize = 0;
	second->cpu1.TestIntr(); second->cpu2.TestIntr();

	while (first->clockcount < 0)
	{
		first->Test();
		second->Test();
	}
		
	return nclocks + first->clockcount;
}

// ----------------------------------------------------------------------------

int Z80Test::Exec(int step)
{
	execcount += clockcount;
	clockcount = -step;

	readcount = writecount = readcountt = writecountt = codesize = 0;
	cpu1.TestIntr();
	cpu2.TestIntr();

	while (clockcount < 0)
		Test();

	return step + clockcount;
}

// ---------------------------------------------------------------------------
//	Exec を途中で中断
//
void Z80Test::Stop(int count)
{
	execcount += clockcount + count;
	clockcount = -count;
}

// ----------------------------------------------------------------------------

void Z80Test::Test()
{
	Z80Reg& reg1 = cpu1.GetReg();
	Z80Reg& reg2 = cpu2.GetReg();

	// 初期化
	readcount = writecount = readcountt = writecountt = codesize = 0;
	reg = reg2 = reg1;					// レジスタを一致させる

	pc = reg1.pc;
	cpu2.SetPC(pc);
	intr = cpu1.IsIntr() + 2 * cpu2.IsIntr();

	// 実行
	clockcount += cpu1.ExecOne();
	cpu2.ExecOne();

	// フラグの一致を確認
	if ((reg1.r.b.flags & 0xd7) != (reg2.r.b.flags & 0xd7))
	{
		Error("フラグの不一致");
		return;
	}

	// レジスタの一致確認
	if (((reg1.r.b.a ^ reg2.r.b.a)
		| (reg1.r.w.bc ^ reg2.r.w.bc)
		| (reg1.r.w.de ^ reg2.r.w.de)
		| (reg1.r.w.hl ^ reg2.r.w.hl)
		| (reg1.r.w.ix ^ reg2.r.w.ix)
		| (reg1.r.w.iy ^ reg2.r.w.iy)
		| (reg1.r.w.sp ^ reg2.r.w.sp)
		| (!reg1.iff1 ^ !reg2.iff1)
		) & 0xffff)
	{
		Error("レジスタの不一致");
		return;
	}

	if (readcount != readcountt)
	{
		Error("読み込み回数の不一致");
		return;
	}

	if (writecount != writecountt)
	{
		Error("書き込み回数の不一致");
		return;
	}

	// PC の一致確認
	if (cpu1.GetPC() != cpu2.GetPC())
	{
		Error("PC の不一致");
		return;
	}
}

// ----------------------------------------------------------------------------

void Z80Test::Error(const char* errtxt)
{
	if (fp)
	{
		if (code[0] == 0xfb)		// special case
			return;
		if (code[0] == 0xed && code[1] == 0xa2)
			return;
		uint i;
		fprintf(fp, "PC: %.4x   ", pc); 
		for (i=0; i<4; i++)
		{
			fprintf(fp, (i>codesize ? "   " : "%.2x "), code[i]);
		}

		Z80Reg& reg1 = cpu1.GetReg();
		Z80Reg& reg2 = cpu2.GetReg();

		fprintf(fp,	"%s  reads:ref=%d target=%d writes:ref=%d target=%d\n", errtxt, readcount, readcountt, writecount, writecountt);
		for (i=0; i<readcount; i++)
		{
			fprintf(fp, "Read %d: %.4x %.2x\n", i, readptr[i], readdat[i]);
		}
		for (i=0; i<writecount; i++)
		{
			fprintf(fp, "Write %d: %.4x %.2x\n", i, writeptr[i], writedat[i]);
		}

		fprintf(fp, "     O PC:%.4x SP:%.4x AF:%.4x HL:%.4x DE:%.4x BC:%.4x IX:%.4x IY:%.4x IFF%d IRQ:%d R:%.2x\n",
					pc, reg.r.w.sp & 0xffff, reg.r.w.af & 0xffd7, reg.r.w.hl & 0xffff,
					reg.r.w.de & 0xffff, reg.r.w.bc & 0xffff, reg.r.w.ix & 0xffff, reg.r.w.iy & 0xffff, reg.iff1, intr, reg.rreg);
		
		fprintf(fp, "     O PC:%.4x SP:%.4x AF:%.4x HL:%.4x DE:%.4x BC:%.4x IX:%.4x IY:%.4x IFF%d IRQ:%d IM:%d R:%.2x\n",
					cpu1.GetPC(), reg1.r.w.sp & 0xffff, reg1.r.w.af & 0xffd7, reg1.r.w.hl & 0xffff,
					reg1.r.w.de & 0xffff, reg1.r.w.bc & 0xffff, reg1.r.w.ix & 0xffff, reg1.r.w.iy & 0xffff, reg1.iff1, cpu1.IsIntr(), reg1.intmode, reg1.rreg);
		
		fprintf(fp, "     O PC:%.4x SP:%.4x AF:%.4x HL:%.4x DE:%.4x BC:%.4x IX:%.4x IY:%.4x IFF%d IRQ:%d IM:%d R:%.2x\n",
					cpu2.GetPC(), reg2.r.w.sp & 0xffff, reg2.r.w.af & 0xffd7, reg2.r.w.hl & 0xffff,
					reg2.r.w.de & 0xffff, reg2.r.w.bc & 0xffff, reg2.r.w.ix & 0xffff, reg2.r.w.iy & 0xffff, reg2.iff1, cpu2.IsIntr(), reg2.intmode, reg2.rreg);
	
	}
}

// ----------------------------------------------------------------------------

inline uint Z80Test::Read8R(uint a)
{
	uint fcount = a - pc;
	if (fcount < 4)
	{
		codesize = (fcount > codesize) ? fcount : codesize;
		return code[fcount] = bus->Read8(a);
	}

	if (readcount < 8)
	{
		uint data = bus->Read8(a);
		readptr[readcount] = a;
		readdat[readcount] = data;
		readcount++;
		return data;
	}
	fprintf(fp, "%x %x\n", a, pc);
	Error("１命令中に８バイトを超えるデータの読み込み");
	return bus->Read8(a);
}

// ----------------------------------------------------------------------------

inline uint Z80Test::Read8T(uint a)
{
	uint fcount = a - pc;
	if (fcount < 4)
		return code[fcount];
	
	readcountt++;
	for (uint i=0; i<readcount; i++)
	{
		if (((readptr[i] ^ a) & 0xffff) == 0)
			return readdat[i];
	}

	char buf[128];
	sprintf(buf, "読み込みアドレスの不一致: %.4x", a);
	Error(buf);
	return 0;
}

// ----------------------------------------------------------------------------

inline void Z80Test::Write8R(uint a, uint d)
{
	d &= 0xff;
	if (writecount < 8)
	{
		writeptr[writecount] = a;
		writedat[writecount] = d;
		writecount ++;
		bus->Write8(a, d);
		return;
	}
	Error("１命令中に８バイトを超えるデータの書き込み");
}

inline void Z80Test::Write8T(uint a, uint d)
{
	d &= 0xff;
	writecountt++;
	for (uint i=0; i<writecount; i++)
	{
		if (((writeptr[i] ^ a) & 0xffff) == 0)
		{
			if (writedat[i] != d)
			{
				char buf[128];
				sprintf(buf, "書き込みデータの不一致 at %.4x:%.2x %.2x", a, writedat[i], d);
				Error(buf);
			}
			return;
		}
	}
	char buf[128];
	sprintf(buf, "書き込みアドレスの不一致: %.4x", a);
	Error(buf);
}

// ----------------------------------------------------------------------------

uint Z80Test::InR(uint a)
{
	uint data = bus->In(a);
	inptr = a, indat = data;
	return data;
}

uint Z80Test::InT(uint a)
{
	if (inptr == a)
		return indat;
	Error("入力ポートの不一致");
	return 0;
}

// ----------------------------------------------------------------------------

void Z80Test::OutR(uint a, uint d)
{
	outptr = a, outdat = d;
	bus->Out(a, d);
}

void Z80Test::OutT(uint a, uint d)
{
	if (outptr == a)
	{
		if (outdat != d)
			Error("出力データの不一致");
		return;
	}
	Error("出力ポートの不一致");
}

// ----------------------------------------------------------------------------

void Z80Test::Reset(uint, uint)
{
	cpu1.Reset();
	cpu2.Reset();
}

void Z80Test::IRQ(uint, uint d)
{
	cpu1.IRQ(0, d);
	cpu2.IRQ(0, d);
}

void Z80Test::NMI(uint, uint d)
{
}

// ----------------------------------------------------------------------------

uint MEMCALL Z80Test::S_Read8R(void* inst, uint a)
{
	return ((Z80Test*)(inst))->Read8R(a);
}

uint MEMCALL Z80Test::S_Read8T(void* inst, uint a)
{
	return ((Z80Test*)(inst))->Read8T(a);
}

void MEMCALL Z80Test::S_Write8R(void* inst, uint a, uint d)
{
	((Z80Test*)(inst))->Write8R(a, d);
}

void MEMCALL Z80Test::S_Write8T(void* inst, uint a, uint d)
{
	((Z80Test*)(inst))->Write8T(a, d);
}

// ---------------------------------------------------------------------------
//	Device descriptor
//	
const Device::Descriptor Z80Test::descriptor =
{
	0, outdef
};

const Device::OutFuncPtr Z80Test::outdef[] =
{
	STATIC_CAST(Device::OutFuncPtr, Reset),
	STATIC_CAST(Device::OutFuncPtr, IRQ),
	STATIC_CAST(Device::OutFuncPtr, NMI),
};
