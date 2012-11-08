// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//	$Id: keybconn.h,v 1.1 2002/04/07 05:40:10 cisc Exp $

#pragma once

// ---------------------------------------------------------------------------

#include "types.h"
#include "device.h"

namespace PC8801
{
	class WinKeyIF;
}

// ---------------------------------------------------------------------------
//	デバイスをバスに接続する為の仲介を行う．
//	デバイスの各機能に対応するポート番号のマッピング情報をもつ．
//	取り外す必要がない場合接続後に破棄してもかまわない．
//	
class DeviceConnector
{
public:
	virtual bool Disconnect();

protected:
	bool Connect(IOBus* bus, Device* dev, const IOBus::Connector* conn);

private:
	IOBus* bus;
	Device* dev;
};

// ---------------------------------------------------------------------------

class KeyboardConnector
{
public:
	bool Connect(IOBus* bus, PC8801::WinKeyIF* keyb);

private:
	IOBus* bus;
	Device* dev;
};

