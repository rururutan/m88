// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  Š¿Žš ROM
// ---------------------------------------------------------------------------
//	$Id: kanjirom.h,v 1.5 1999/10/10 01:47:07 cisc Exp $

#pragma once

#include "device.h"

namespace PC8801
{

class KanjiROM : public Device  
{
public:
	enum
	{
		setl = 0, seth
	};
	
	enum
	{
		readl = 0, readh
	};

public:
	KanjiROM(const ID& id);
	~KanjiROM();

	bool Init(const char* filename);

	void IOCALL SetL(uint p, uint d);
	void IOCALL SetH(uint p, uint d);
	uint IOCALL ReadL(uint p);
	uint IOCALL ReadH(uint p);
	
	uint IFCALL GetStatusSize() { return sizeof(uint); }
	bool IFCALL SaveStatus(uint8* status) { *(uint*) status = adr; return true; }
	bool IFCALL LoadStatus(const uint8* status) { adr = *(const uint*) status; return true; }

	const Descriptor* IFCALL GetDesc() const { return &descriptor; } 
	
private:
	uint adr;
	uint8* image;

	static const Descriptor descriptor;
	static const InFuncPtr indef[];
	static const OutFuncPtr outdef[];
};

};

