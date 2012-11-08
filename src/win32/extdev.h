// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: extdev.h,v 1.6 1999/11/26 10:14:09 cisc Exp $

#pragma once

#include "device.h"
#include "pc88/pcinfo.h"
#include "pc88/sound.h"

class PC88;
class Scheduler;

namespace PC8801
{
	class Memory;
	class PD8257;
}

namespace PC8801
{

class ExternalDevice : public Device, public ISoundSource
{
public:
	ExternalDevice();
	~ExternalDevice();
	
	bool Init(const char* dllname, PC88* pc, IOBus* bus, PD8257* dmac, ISoundControl* sound, IMemoryManager* mm);
	bool Cleanup();

	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);
	
private:
	typedef void* (__cdecl *F_CONNECT)(void*, const	PCInfo*, DeviceInfo*);
	typedef bool (__cdecl *F_DISCONNECT)(void*);

private:
	bool InitPCInfo();
	bool LoadDLL(const char* dllname);
	
	void IOCALL EventProc(uint arg);
	uint IOCALL In(uint port);
	void IOCALL Out(uint port, uint data);
	
	bool IFCALL SetRate(uint r);
	void IFCALL Mix(int32* s, int len);
	bool IFCALL Connect(ISoundControl* sound) { return false; }

	HMODULE hdll;
	IOBus* bus;
	PD8257* dmac;
	PC88* pc;
	ISoundControl* sound;
	IMemoryManager* mm;
	int mid;
	
	void* dev;
	
	F_CONNECT f_connect;
	F_DISCONNECT f_disconnect;
	
	DeviceInfo devinfo;
	static PCInfo pcinfo;

private:
	static int S_DMARead(void*, uint bank, uint8* data, uint nbytes);
	static int S_DMAWrite(void*, uint bank, uint8* data, uint nbytes);
	static bool S_MemAcquire(void*, uint page, uint npages, void* read, void* write, uint flags);
	static bool S_MemRelease(void*, uint page, uint npages, uint flags);
	static void* S_AddEvent(void*, uint count, uint arg);
	static bool S_DelEvent(void*, void*);
	static uint S_GetTime(void*);
	static void S_SoundUpdate(void*);
};

}

