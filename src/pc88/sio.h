// ---------------------------------------------------------------------------
//	M88 - PC-8801 Series Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	Implementation of USART(uPD8251AF)
// ---------------------------------------------------------------------------
//	$Id: sio.h,v 1.5 2000/06/26 14:05:30 cisc Exp $

#pragma once

#include "device.h"

class Scheduler;

namespace PC8801
{

class SIO : public Device
{
public:
	enum
	{
		reset=0, setcontrol, setdata, acceptdata,
		getstatus=0, getdata,
	};

public:
	SIO(const ID& id);
	~SIO();
	bool Init(IOBus* bus, uint prxrdy, uint prequest);

	void IOCALL Reset(uint=0, uint=0);
	void IOCALL SetControl(uint, uint d);
	void IOCALL SetData(uint, uint d);
	uint IOCALL GetStatus(uint=0);
	uint IOCALL GetData(uint=0);

	void IOCALL AcceptData(uint, uint);

	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* s);
	bool IFCALL LoadStatus(const uint8* s);

	const Descriptor* IFCALL GetDesc() const { return &descriptor; }

private:
	enum Mode { clear=0, async, sync1, sync2, sync };
	enum Parity { none='N', odd='O', even='E' };

	IOBus* bus;
	uint prxrdy;
	uint prequest;
	
	uint baseclock;
	uint clock;
	uint datalen;
	uint stop;
	uint status;
	uint data;
	Mode mode;
	Parity parity;
	bool rxen;
	bool txen;

private:
	enum
	{
		TXRDY	= 0x01,
		RXRDY	= 0x02,
		TXE		= 0x04,
		PE		= 0x08,
		OE		= 0x10,
		FE		= 0x20,
		SYNDET	= 0x40,
		DSR		= 0x80,
	
		ssrev	= 1,
	};
	struct Status
	{
		uint8	rev;
		bool	rxen;
		bool	txen;

		uint baseclock;
		uint clock;
		uint datalen;
		uint stop;
		uint status;
		uint data;
		Mode mode;
		Parity parity;
	};

private:
	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};

}

