// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//	$Id: keybconn.cpp,v 1.1 2002/04/07 05:40:10 cisc Exp $

#include "headers.h"
#include "keybconn.h"
#include "winkeyif.h"
#include "pc88/pc88.h"

#define LOGNAME "wincore"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------

bool DeviceConnector::Connect(IOBus* _bus, Device* _dev, IOBus::Connector* conn)
{
	bus = _bus;
	dev = _dev;

	if (!bus->Connect(dev, conn)) 
		return false;
	return true;
}

bool DeviceConnector::Disconnect()
{
	bool r = false;
	if (bus)
		r = bus->Disconnect(dev);
	bus = 0;
	dev = 0;
	return r;
}

// ---------------------------------------------------------------------------
//	Windows 用のデバイスを接続
//
bool KeyboardConnector::Connect(IOBus* bus, WinKeyIF* keyb)
{
	static const IOBus::Connector c_keyb[] = 
	{
		{ PC88::pres, IOBus::portout, WinKeyIF::reset },
		{ PC88::vrtc, IOBus::portout, WinKeyIF::vsync },
		{ 0x00, IOBus::portin, WinKeyIF::in },
		{ 0x01, IOBus::portin, WinKeyIF::in },
		{ 0x02, IOBus::portin, WinKeyIF::in },
		{ 0x03, IOBus::portin, WinKeyIF::in },
		{ 0x04, IOBus::portin, WinKeyIF::in },
		{ 0x05, IOBus::portin, WinKeyIF::in },
		{ 0x06, IOBus::portin, WinKeyIF::in },
		{ 0x07, IOBus::portin, WinKeyIF::in },
		{ 0x08, IOBus::portin, WinKeyIF::in },
		{ 0x09, IOBus::portin, WinKeyIF::in },
		{ 0x0a, IOBus::portin, WinKeyIF::in },
		{ 0x0b, IOBus::portin, WinKeyIF::in },
		{ 0x0c, IOBus::portin, WinKeyIF::in },
		{ 0x0d, IOBus::portin, WinKeyIF::in },
		{ 0x0e, IOBus::portin, WinKeyIF::in },
		{ 0x0f, IOBus::portin, WinKeyIF::in },
		{ 0, 0, 0 }
	};
	return Connect(bus, keyb, c_keyb);
}

