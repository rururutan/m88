// ---------------------------------------------------------------------------
//	M88 - PC-8801 Series Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	Implementation of USART(uPD8251AF)
// ---------------------------------------------------------------------------
//	$Id: sio.cpp,v 1.6 2001/02/21 11:57:57 cisc Exp $

#include "headers.h"
#include "schedule.h"
#include "sio.h"

#define LOGNAME "sio"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築・破棄
//
SIO::SIO(const ID& id)
: Device(id)
{
}

SIO::~SIO()
{
}

// ---------------------------------------------------------------------------
//	初期化
//
bool SIO::Init(IOBus* _bus, uint _prxrdy, uint _preq)
{
	bus = _bus, prxrdy = _prxrdy, prequest = _preq;
	LOG0("SIO::Init\n");

	return true;
}

// ---------------------------------------------------------------------------
//	りせっと
//
void SIO::Reset(uint, uint)
{
	mode = clear;
	status = TXRDY | TXE;
	baseclock = 1200 * 64;
}

// ---------------------------------------------------------------------------
//	こんとろーるぽーと
//
void IOCALL SIO::SetControl(uint, uint d)
{
	LOG1("[%.2x] ", d);
	
	switch (mode)
	{
	case clear:
		// Mode Instruction
		if (d & 3)
		{
			// Asynchronus mode
			mode = async;
			// b7 b6 b5 b4 b3 b2 b1 b0
			// STOP  EP PE CHAR  RATE
			static const int clockdiv[] = { 1, 1, 16, 64 };
			clock = baseclock / clockdiv[d & 3];
			datalen = 5 + ((d >> 2) & 3);
			parity = d & 0x10 ? (d & 0x20 ? even : odd) : none;
			stop = (d >> 6) & 3;
			LOG4("Async: %d baud, Parity:%c Data:%d Stop:%s\n", clock, parity, datalen, stop==3 ? "2" : stop==2 ? "1.5" : "1");
		}
		else
		{
			// Synchronus mode
			mode = sync1;
			clock = 0;
			parity = d & 0x10 ? (d & 0x20 ? even : odd) : none;
			LOG2("Sync: %d baud, Parity:%c / ", clock, parity);
		}
		break;
		
	case sync1:
		mode = sync2;
		break;

	case sync2:
		mode = sync;
		LOG0("\n");
		break;

	case async:
	case sync:
		// Command Instruction
		// b7 - enter hunt mode
		// b6 - internal reset
		if (d & 0x40)
		{
			// Reset!
			LOG0(" Internal Reset!\n");
			mode = clear;
			break;
		}
		// b5 - request to send
		// b4 - error reset
		if (d & 0x10)
		{
			LOG0(" ERRCLR"); 
			status &= ~(PE | OE | FE);
		}
		// b3 - send break charactor
		if (d & 8)
		{
			LOG0(" SNDBRK");
		}
		// b2 - receive enable 
		rxen = (d & 4) != 0;
		// b1 - data terminal ready
		// b0 - send enable
		txen = (d & 1) != 0;

		LOG2(" RxE:%d TxE:%d\n", rxen, txen);
		break;
	default:
		LOG1("internal error? <%d>\n", mode);
		break;
	}
}

// ---------------------------------------------------------------------------
//	でーたせっと
//
void IOCALL SIO::SetData(uint, uint d)
{
	LOG1("<%.2x ", d);
}

// ---------------------------------------------------------------------------
//	じょうたいしゅとく
//
uint IOCALL SIO::GetStatus(uint)
{
//	LOG1("!%.2x ", status      );
	return status;
}

// ---------------------------------------------------------------------------
//	でーたしゅとく
//
uint IOCALL SIO::GetData(uint)
{
	LOG1(">%.2x ", data);
	
	int f = status & RXRDY;
	status &= ~RXRDY;

	if (f)
		bus->Out(prequest, 0);

	return data;
}

void IOCALL SIO::AcceptData(uint, uint d)
{
	LOG1("Accept: [%.2x]", d);
	if (rxen)
	{
		data = d;
		if (status & RXRDY)
		{
			status |= OE;
			LOG0(" - overrun");
		}
		status |= RXRDY;
		bus->Out(prxrdy, 1);		// 割り込み
		LOG0("\n");
	}
	else
	{
		LOG0(" - ignored\n");
	}
}

// ---------------------------------------------------------------------------
//	状態保存
//
uint IFCALL SIO::GetStatusSize()
{
	return sizeof(Status);
}

bool IFCALL SIO::SaveStatus(uint8* s)
{
	Status* status = (Status*) s;
	status->rev			= ssrev;
	status->baseclock	= baseclock;
	status->clock		= clock;
	status->datalen		= datalen;
	status->stop		= stop;
	status->data		= data;
	status->mode		= mode;
	status->parity		= parity;
	return true;
}

bool IFCALL SIO::LoadStatus(const uint8* s)
{
	const Status* status = (const Status*) s;
	if (status->rev != ssrev)
		return false;
	baseclock	= status->baseclock;
	clock		= status->clock;
	datalen		= status->datalen;
	stop		= status->stop;
	data		= status->data;
	mode		= status->mode;
	parity		= status->parity;
	return true;
}


// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor SIO::descriptor = { indef, outdef };

const Device::OutFuncPtr SIO::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &Reset),
	STATIC_CAST(Device::OutFuncPtr, &SetControl),
	STATIC_CAST(Device::OutFuncPtr, &SetData),
	STATIC_CAST(Device::OutFuncPtr, &AcceptData),
};

const Device::InFuncPtr SIO::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, &GetStatus),
	STATIC_CAST(Device::InFuncPtr, &GetData),
};

