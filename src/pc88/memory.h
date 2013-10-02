// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	Main ë§ÉÅÉÇÉä(ä‹ALU)ÇÃé¿ëï
// ----------------------------------------------------------------------------
//	$Id: memory.h,v 1.26 2003/09/28 14:58:54 cisc Exp $

#pragma once

#include "device.h"
#include "config.h"

class MemoryManager;

namespace PC8801
{
class CRTC;

class Memory : public Device, public IGetMemoryBank
{
public:
	enum IDOut
	{
		reset=0, out31, out32, out34, out35, out40,
		  out5x, out70, out71, out78, out99, oute2,
		  oute3, outf0, outf1, vrtc, out33
	};
	enum IDIn
	{
		in32=0, in5c, in70, in71, ine2, ine3, in33
	};
	union quadbyte
	{
		uint32 pack;
		uint8 byte[4];
	};
	enum ROM { n88 = 0, n88e = 0x8000, n80 = 0x10000, romsize = 0x18000 };

	enum MemID
	{
		mRAM, mTV,
		mN88, mN, mN80, mN80SR, mN80SR1, mN88E0, mN88E1, mN88E2, mN88E3,
		mG0, mG1, mG2, mALU, mCD0, mCD1, mJISYO,
		mE1, mE2, mE3, mE4, mE5, mE6, mE7, 
		mERAM,
	};

public:
	Memory(const ID& id);
	~Memory();
	const Descriptor* IFCALL GetDesc() const { return &descriptor; } 
	
	void ApplyConfig(const Config* cfg);
	uint8* GetRAM() { return ram; }
	uint8* GetERAM( uint bank ) { return ((bank < erambanks) ? &eram[bank * 0x8000] : ram); }
	uint8* GetTVRAM() { return tvram; }
	quadbyte* GetGVRAM() { return gvram; }
	uint8* GetROM() { return rom; }
	uint8* GetDirtyFlag() { return dirty; }
	
	uint IFCALL GetRdBank(uint addr);
	uint IFCALL GetWrBank(uint addr);
	
	uint IFCALL GetStatusSize();
	bool IFCALL LoadStatus(const uint8* status);
	bool IFCALL SaveStatus(uint8* status);
	bool IsN80Ready() { return !!n80rom; }
	bool IsN80V2Ready() { return !!n80v2rom; }
	bool IsCDBIOSReady() { return !!cdbios; }

	bool Init(MemoryManager* mgr, IOBus* bus, CRTC* crtc, int* waittbl);
	void IOCALL Reset(uint, uint);
	void IOCALL Out31(uint, uint data);
	void IOCALL Out32(uint, uint data);
	void IOCALL Out33(uint, uint data);
	void IOCALL Out34(uint, uint data);
	void IOCALL Out35(uint, uint data);
	void IOCALL Out40(uint, uint data);
	void IOCALL Out5x(uint port, uint);
	void IOCALL Out70(uint, uint data);
	void IOCALL Out71(uint, uint data);
	void IOCALL Out78(uint, uint data);
	void IOCALL Out99(uint, uint data);
	void IOCALL Oute2(uint, uint data);
	void IOCALL Oute3(uint, uint data);
	void IOCALL Outf0(uint, uint data);
	void IOCALL Outf1(uint, uint data);
	uint IOCALL In32(uint);
	uint IOCALL In33(uint);
	uint IOCALL In5c(uint);
	uint IOCALL In70(uint);
	uint IOCALL In71(uint);
	uint IOCALL Ine2(uint);
	uint IOCALL Ine3(uint);
	void IOCALL VRTC(uint, uint data);
	
private:
	struct WaitDesc
	{
		uint b0, bc, bf;
	};

	enum
	{
		ssrev = 2,			// Status ÇçXêVéûÇ…ëùÇ‚Ç∑Ç±Ç∆
	};
	struct Status
	{
		uint8 rev;
		uint8 p31, p32, p33, p34, p35, p40, p5x;
		uint8 p70, p71, p99, pe2, pe3, pf0;
		quadbyte alureg;
		
		uint8 ram[0x10000];
		uint8 tvram[0x1000];
		uint8 gvram[3][0x4000];
		uint8 eram[1];
	};

	bool InitMemory();
	bool LoadROM();
	bool LoadROMImage(uint8* at, const char* file, int length);
	bool LoadOptROM(const char* file, uint8*& rom, int length);
	void SetWait();
	void SetWaits(uint, uint, uint);
	void SelectJisyo();

	void Update00R();
	void Update60R();
	void Update00W();
	void Update80();
	void UpdateC0();
	void UpdateF0();
	void UpdateN80W();
	void UpdateN80R();
	void UpdateN80G();
	void SelectGVRAM(uint top);
	void SelectALU(uint top);
	void SetRAMPattern(uint8* ram, uint length);

	uint GetHiBank(uint addr);
	
	MemoryManager* mm;
	int mid;
	int* waits;
	IOBus* bus;
	CRTC* crtc;
	uint8* rom;
	uint8* ram;
	uint8* eram;
	uint8* tvram;
	uint8* dicrom;		// é´èëROM
	uint8* cdbios;		// CD-ROM BIOS ROM
	uint8* n80rom;		// N80-BASIC ROM
	uint8* n80v2rom;	// N80SR
	uint8* erom[8+1];	// ägí£ ROM

	uint port31, port32, port33, port34, port35, port40, port5x;
	uint port99, txtwnd, port71, porte2, porte3, portf0;
	uint sw31;
	uint erommask;
	uint waitmode;
	uint waittype;	// b0 = disp/vrtc, 
	bool selgvram;
	bool seldic;
	bool enablewait;
	bool n80mode;
	bool n80srmode;
	uint erambanks;
	uint neweram;
	uint8* r00;
	uint8* r60;
	uint8* w00;
	uint8* rc0;
	quadbyte alureg;
	quadbyte maskr, maski, masks, aluread;
	
	quadbyte gvram[0x4000];
	uint8 dirty[0x400];
	
	static const WaitDesc waittable[48];

	static void MEMCALL WrWindow(void* inst, uint addr, uint data);
	static uint MEMCALL RdWindow(void* inst, uint addr);

	static void MEMCALL WrGVRAM0(void* inst, uint addr, uint data);
	static void MEMCALL WrGVRAM1(void* inst, uint addr, uint data);
	static void MEMCALL WrGVRAM2(void* inst, uint addr, uint data);
	static uint MEMCALL RdGVRAM0(void* inst, uint addr);
	static uint MEMCALL RdGVRAM1(void* inst, uint addr);
	static uint MEMCALL RdGVRAM2(void* inst, uint addr);

	static void MEMCALL WrALUSet(void* inst, uint addr, uint data);
	static void MEMCALL WrALURGB(void* inst, uint addr, uint data);
	static void MEMCALL WrALUR(void* inst, uint addr, uint data);
	static void MEMCALL WrALUB(void* inst, uint addr, uint data);
	static uint MEMCALL RdALU(void* inst, uint addr);

	static const Descriptor descriptor;
	static const InFuncPtr indef[];
	static const OutFuncPtr outdef[];
};

}; // namespace PC8801

