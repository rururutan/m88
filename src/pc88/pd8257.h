// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator.
//	Copyright (C) cisc 1998.
// ---------------------------------------------------------------------------
//  DMAC (uPD8257) のエミュレーション
// ---------------------------------------------------------------------------
//	$Id: pd8257.h,v 1.10 1999/10/10 01:47:10 cisc Exp $

#pragma once

#include "device.h"
#include "if/ifpc88.h"

// ---------------------------------------------------------------------------

namespace PC8801
{

class PD8257 : public Device, public IDMAAccess
{
public:
	enum IDOut
	{
		reset=0, setaddr, setcount, setmode
	};
	enum IDIn
	{
		getaddr, getcount, getstat
	};

public:
	PD8257(const ID&);
	~PD8257();
	
	bool ConnectRd(uint8* mem, uint addr, uint length);
	bool ConnectWr(uint8* mem, uint addr, uint length);
	
	void IOCALL Reset(uint=0, uint=0);
	void IOCALL SetAddr (uint port, uint data);
	void IOCALL SetCount(uint port, uint data);
	void IOCALL SetMode (uint, uint data);
	uint IOCALL GetAddr (uint port);
	uint IOCALL GetCount(uint port);
	uint IOCALL GetStatus(uint);
	
	uint IFCALL RequestRead(uint bank, uint8* data, uint nbytes);
	uint IFCALL RequestWrite(uint bank, uint8* data, uint nbytes);
	
	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);

	const Descriptor* IFCALL GetDesc() const { return &descriptor; } 

private:
	enum
	{
		ssrev = 1,
	};
	struct Status
	{
		uint8 rev;
		bool autoinit;
		bool ff;
		uint8 status;
		uint8 enabled;
		uint addr[4];
		int count[4];
		uint ptr[4];
		uint8 mode[4];
	};

	Status stat;

	uint8* mread;
	uint mrbegin, mrend;
	uint8* mwrite;
	uint mwbegin, mwend;

	static const Descriptor descriptor;
	static const InFuncPtr indef[];
	static const OutFuncPtr outdef[];
};

} // namespace PC8801

