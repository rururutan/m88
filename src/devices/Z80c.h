// ---------------------------------------------------------------------------
//	Z80 emulator in C++
//	Copyright (C) cisc 1997, 1999.
// ----------------------------------------------------------------------------
//	$Id: Z80c.h,v 1.26 2001/02/21 11:57:16 cisc Exp $

#ifndef Z80C_h
#define Z80C_h

#include "types.h"
#include "device.h"
#include "memmgr.h"
#include "Z80.h"
#include "Z80diag.h"

class IOBus;

#define Z80C_STATISTICS

// ----------------------------------------------------------------------------
//	Z80 Emulator
//	
//	使用可能な機能
//	Reset
//	INT
//	NMI
//	
//	bool Init(MemoryManager* mem, IOBus* bus)
//	Z80 エミュレータを初期化する
//	in:		bus		CPU をつなぐ Bus
//	out:			問題なければ true
//	
//	uint Exec(uint clk)
//	指定したクロック分だけ命令を実行する
//	in:		clk		実行するクロック数
//	out:			実際に実行したクロック数
//	
//	int Stop(int clk)
//	実行残りクロック数を変更する
//	in:		clk
//
//	uint GetCount()
//	通算実行クロックカウントを取得
//	out:
//
//	void Reset()
//	Z80 CPU をリセットする
//
//	void INT(int flag)
//	Z80 CPU に INT 割り込み要求を出す
//	in:		flag	true: 割り込み発生
//					false: 取り消し
//	
//	void NMI()
//	Z80 CPU に NMI 割り込み要求を出す
//	
//	void Wait(bool wait)
//	Z80 CPU の動作を停止させる
//	in:		wait	止める場合 true
//					wait 状態の場合 Exec が命令を実行しないようになる
//
class Z80C : public Device
{
public:
	enum
	{
		reset = 0, irq, nmi,
	};

	struct Statistics
	{
		uint execute[0x10000];

		void Clear()
		{
			memset(execute, 0, sizeof(execute));
		}
	};

public:
	Z80C(const ID& id);
	~Z80C();
	
	const Descriptor* IFCALL GetDesc() const { return &descriptor; } 
	
	bool Init(MemoryManager* mem, IOBus* bus, int iack);
	
	int Exec(int count);
	int ExecOne();
	static int ExecSingle(Z80C* first, Z80C* second, int count);
	static int ExecDual(Z80C* first, Z80C* second, int count);
	static int ExecDual2(Z80C* first, Z80C* second, int count);
	
	void Stop(int count);
	static void StopDual(int count) { if (currentcpu) currentcpu->Stop(count); }
	int GetCount();
	static int GetCCount() { return currentcpu ? currentcpu->GetCount() - currentcpu->startcount : 0; }
	
	void IOCALL Reset(uint=0, uint=0);
	void IOCALL IRQ(uint, uint d) { intr = d; }
	void IOCALL NMI(uint=0, uint=0);
	void Wait(bool flag);
	
	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);
	
	uint GetPC();
	void SetPC(uint newpc);
	const Z80Reg& GetReg() { return reg; }

	bool GetPages(MemoryPage** rd, MemoryPage** wr) { *rd = rdpages, *wr = wrpages; return true; }
	int* GetWaits() { return 0; }
	
	void TestIntr();
	bool IsIntr() { return !!intr; }
	bool EnableDump(bool dump);
	int GetDumpState() { return !!dumplog; }

	Statistics* GetStatistics();

		
private:
	enum
	{
		pagebits = MemoryManagerBase::pagebits,
		pagemask = MemoryManagerBase::pagemask,
		idbit	 = MemoryManagerBase::idbit
	};

	enum
	{
		ssrev = 1,
	};
	struct Status
	{
		Z80Reg reg;
		uint8 intr;
		uint8 wait;
		uint8 xf;
		uint8 rev;
		int execcount;
	};

	void DumpLog();

	uint8* inst;		// PC の指すメモリのポインタ，または PC そのもの
	uint8* instlim;		// inst の有効上限
	uint8* instbase;	// inst - PC		(PC = inst - instbase)
	uint8* instpage;
	
	Z80Reg reg;
	IOBus* bus;
	static const Descriptor descriptor;
	static const OutFuncPtr outdef[];
	static Z80C* currentcpu;
	static int cbase;

	int execcount;
	int clockcount;
	int stopcount;
	int delaycount;
	int intack;
	int intr;
	int waitstate;				// b0:HALT b1:WAIT
	int eshift;
	int startcount;
	
	enum index { USEHL, USEIX, USEIY };
	index index_mode;						/* HL/IX/IY どれを参照するか */
	uint8 uf;								/* 未計算フラグ */
	uint8 nfa;								/* 最後の加減算の種類 */
	uint8 xf;								/* 未定義フラグ(第3,5ビット) */
	uint32 fx32, fy32;						/* フラグ計算用のデータ */
	uint fx, fy;
	
	uint8* ref_h[3];						/* H / XH / YH のテーブル */
	uint8* ref_l[3];						/* L / YH / YL のテーブル */
	Z80Reg::wordreg* ref_hl[3];				/* HL/ IX / IY のテーブル */
	uint8* ref_byte[8];						/* BCDEHL A のテーブル */
	FILE* dumplog;
	Z80Diag diag;

	MemoryPage rdpages[0x10000 >> MemoryManager::pagebits];
	MemoryPage wrpages[0x10000 >> MemoryManager::pagebits];

#ifdef Z80C_STATISTICS
	Statistics statistics;
#endif
	
	// 内部インターフェース
private:
	uint Read8(uint addr);
	uint Read16(uint a);
	uint Fetch8();
	uint Fetch16();
	void Write8(uint addr, uint data);
	void Write16(uint a, uint d);
	uint Inp(uint port);
	void Outp(uint port, uint data);
	uint Fetch8B();
	uint Fetch16B();

	void SingleStep(uint inst);
	void SingleStep();
	void Init();
	int  Exec0(int stop, int d);
	int  Exec1(int stop, int d);
	bool Sync();
	void OutTestIntr();

	void SetPCi(uint newpc);
	void PCInc(uint inc);
	void PCDec(uint dec);
	
	void Call(), Jump(uint dest), JumpR();
	uint8 GetCF(), GetZF(), GetSF();
	uint8 GetHF(), GetPF();
	void SetM(uint n);
	uint8 GetM();
	void Push(uint n);
	uint Pop();
	void ADDA(uint8), ADCA(uint8), SUBA(uint8);
	void SBCA(uint8), ANDA(uint8), ORA(uint8);
	void XORA(uint8), CPA(uint8);
	uint8 Inc8(uint8), Dec8(uint8);
	uint ADD16(uint x, uint y);
	void ADCHL(uint y), SBCHL(uint y);
	uint GetAF();
	void SetAF(uint n);
	void SetZS(uint8 a), SetZSP(uint8 a);
	void CPI(), CPD();
	void CodeCB();

	uint8 RLC(uint8), RRC(uint8), RL (uint8);
	uint8 RR (uint8), SLA(uint8), SRA(uint8);
	uint8 SLL(uint8), SRL(uint8);
};

// ---------------------------------------------------------------------------
//  クロックカウンタ取得
//
inline int Z80C::GetCount()
{
	return execcount + (clockcount << eshift);
}

inline uint Z80C::GetPC()
{
	return (uint)(inst - instbase);
}


inline Z80C::Statistics* Z80C::GetStatistics()
{
#ifdef Z80C_STATISTICS
	return &statistics;
#else
	return 0;
#endif
}

#endif // Z80C.h
