// ---------------------------------------------------------------------------
//	Z80 Disassembler
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: Z80diag.h,v 1.3 1999/10/10 01:46:26 cisc Exp $
 
#ifndef Z80DIAG_H
#define Z80DIAG_H

#include "types.h"
#include "device.h"

// ---------------------------------------------------------------------------

class Z80Diag
{
public:
	Z80Diag();
	bool Init(IMemoryAccess* bus);
	uint Disassemble(uint pc, char* dest);
	uint DisassembleS(uint pc, char* dest);
	uint InstInc(uint ad);
	uint InstDec(uint ad);

private:
	enum XMode { usehl=0, useix=2, useiy=4 };
	
	char* Expand(char* dest, const char* src);
	uint8 Read8(uint addr) { return mem->Read8(addr & 0xffff); }
	
	static void SetHex(char*& dest, uint n);
	int GetInstSize(uint ad);
	uint InstCheck(uint ad);
	uint InstDecSub(uint ad, int depth);

	IMemoryAccess* mem;
	uint pc;
	XMode xmode;

	static const char* Inst[0x100];
	static const char* InstED1[0x40];
	static const char* InstED2[0x10];
	static const char* InstCB[0x20];
	static const int8 SizeTable[0x100];
	static const int8 SizeTableED[0x80];
};

#endif // Z80DIAG_H
