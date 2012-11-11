// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1997, 2000.
// ---------------------------------------------------------------------------
//	$Id: tapemgr.h,v 1.2 2000/06/26 14:05:30 cisc Exp $

#pragma once

#include "device.h"
#include "opna.h"
#include "psg.h"
#include "soundbuf.h"
#include "schedule.h"

// ---------------------------------------------------------------------------
//
//
class TapeManager : public Device
{
public:
	enum
	{
		requestdata = 0, out30,
		in40 = 0,
	};

	TapeManager();
	~TapeManager();

	bool Init(Scheduler* s, IOBus* bus, int pin);

	bool Open(const char* file);
	bool Close();
	bool Rewind(bool timer = true);

	bool IsOpen() { return !!tags; }

	bool Motor(bool on);
	bool Carrier();
	
	bool Seek(uint pos, uint offset);
	uint GetPos();

	uint ReadByte();
	void IOCALL RequestData(uint=0, uint=0);

	void IOCALL Out30(uint, uint en);
	uint IOCALL In40(uint);

	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);
	
	const Descriptor* IFCALL GetDesc() const { return &descriptor; }
	
private:
	enum
	{
		T88VER		= 0x100,
		ssrev		= 1,
	};
	enum Mode
	{
		T_END		= 0,
		T_VERSION	= 1,
		T_BLANK		= 0x100,
		T_DATA		= 0x101,
		T_SPACE		= 0x102,
		T_MARK		= 0x103,
	};
	struct TagHdr
	{
		uint16 id;
		uint16 length;
	};
	struct Tag
	{
		Tag* next;
		Tag* prev;
		uint16 id;
		uint16 length;
		uint8 data[1];
	};
	struct BlankTag
	{
		uint32 pos;
		uint32 tick;
	};
	struct DataTag
	{
		uint32 pos;
		uint32 tick;
		uint16 length;
		uint16 type;
		uint8 data[1];
	};
	struct Status
	{
		uint8 rev;
		bool motor;
		uint32 pos;
		uint offset;
	};
	
	void Proceed(bool timer = true);
	void IOCALL Timer(uint=0);
	void Send(uint);
	void SetTimer(int t);

	Scheduler* scheduler;
	SchedulerEvent* event;
	Tag* tags;
	Tag* pos;
	int offset;
	uint32 tick;
	Mode mode;
	uint time;			// motor on: タイマー開始時間
	uint timercount;
	uint timerremain;	// タイマー残り
	bool motor;

	IOBus* bus;
	int pinput;

	uint8* data;
	int datasize;
	int datatype;

private:
	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};

