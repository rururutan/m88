// ---------------------------------------------------------------------------
//	PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	äÑÇËçûÇ›ÉRÉìÉgÉçÅ[Éâé¸ÇË(É PD8214)
// ---------------------------------------------------------------------------
//	$Id: intc.h,v 1.7 1999/10/10 01:47:06 cisc Exp $

#pragma once

#include "device.h"

namespace PC8801
{

// ---------------------------------------------------------------------------

class INTC : public Device
{
public:
	enum
	{
		reset=0, request, setmask, setreg,
		intack=0
	};

public:
	INTC(const ID& id);
	~INTC();
	bool Init(IOBus* bus, uint irqport, uint ipbase);
	
	void IOCALL Reset(uint=0, uint=0);
	void IOCALL Request(uint port, uint en);
	void IOCALL SetMask(uint, uint data);
	void IOCALL SetRegister(uint, uint data);
	uint IOCALL IntAck(uint);

	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);

	const Descriptor* IFCALL GetDesc() const { return &descriptor; } 

private:
	struct Status
	{
		uint mask;
		uint mask2;
		uint irq;
	};
	void IRQ(bool);
	
	IOBus* bus;
	Status stat;
	uint irqport;
	uint iportbase;

	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};

}

