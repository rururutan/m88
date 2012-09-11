// ---------------------------------------------------------------------------
//	PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: subsys.cpp,v 1.13 2000/02/29 12:29:52 cisc Exp $

#include "headers.h"
#include "device.h"
#include "device_i.h"
#include "subsys.h"
#include "file.h"
#include "status.h"
#include "memmgr.h"

//#define LOGNAME "subsys"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築・破棄
//
SubSystem::SubSystem(const ID& id)
: Device(id), mm(0), mid(-1), rom(0)
{
	cw_m = cw_s = 0x80;
}

SubSystem::~SubSystem()
{
	if (mid != -1)
		mm->Disconnect(mid);
	delete[] rom;
}

// ---------------------------------------------------------------------------
//	初期化
//
bool SubSystem::Init(MemoryManager* _mm)
{
	mm = _mm;
	mid = mm->Connect(this);
	if (mid == -1)
		return false;
	
	if (!InitMemory())
		return false;
	piom.Connect(&pios);
	pios.Connect(&piom);
	return true;
}

// ---------------------------------------------------------------------------
//	メモリ初期化
//
bool SubSystem::InitMemory()
{
	const uint pagesize = 1 << MemoryManagerBase::pagebits;
	
	if (!rom)
		rom = new uint8[0x2000 + 0x4000 + 2 * pagesize];
	ram = rom + 0x2000;
	dummy = ram + 0x4000;
	if (!rom)
		return false;
	memset(dummy, 0xff, pagesize);
	memset(ram, 0x00, 0x4000);
	
	LoadROM();
	PatchROM();

	// map dummy memory
	for (int i=0; i<0x10000; i+=pagesize)
	{
		mm->AllocR(mid, i, pagesize, dummy);
		mm->AllocW(mid, i, pagesize, dummy + pagesize);
	}

	mm->AllocR(mid, 0, 0x2000, rom);
	mm->AllocR(mid, 0x4000, 0x4000, ram);
	mm->AllocW(mid, 0x4000, 0x4000, ram);
	return true;
}

// ---------------------------------------------------------------------------
//	ROM 読み込み
//
bool SubSystem::LoadROM()
{
	memset(rom, 0xff, 0x2000);
	
	FileIO fio;
	if (fio.Open("PC88.ROM", FileIO::readonly))
	{
		fio.Seek(0x14000, FileIO::begin);
		fio.Read(rom, 0x2000);
		return true;
	}
	if (fio.Open("DISK.ROM", FileIO::readonly))
	{
		fio.Seek(0, FileIO::begin);
		fio.Read(rom, 0x2000);
		return true;
	}
	rom[0] = 0xf3;
	rom[1] = 0x76;
	return false;
}

// ---------------------------------------------------------------------------
//	SubSystem::PatchROM
//	モーターの回転待ちを省略するパッチを当てる
//	別にあてなくても起動が遅くなるだけなんだけどね．
//
void SubSystem::PatchROM()
{
	if (rom[0xfb] == 0xcd && rom[0xfc] == 0xb4 && rom[0xfd] == 0x02)
	{
		rom[0xfb] = rom[0xfc] = rom[0xfd] = 0;
		rom[0x105] = rom[0x106] = rom[0x107] = 0;
	}
}

// ---------------------------------------------------------------------------
//	Reset
//
void IOCALL SubSystem::Reset(uint, uint)
{
	piom.Reset();
	pios.Reset();
	idlecount = 0;
}

// ---------------------------------------------------------------------------
//	割り込み受理
//
uint IOCALL SubSystem::IntAck(uint)
{
	return 0x00;
}

// ---------------------------------------------------------------------------
//	Main 側 PIO
//
void IOCALL SubSystem::M_Set0(uint, uint data)
{
	idlecount = 0;
	LOG1(".%.2x ", data);
	piom.SetData(0, data);
}

void IOCALL SubSystem::M_Set1(uint, uint data)
{
	idlecount = 0;
	LOG1(" %.2x ", data);
	piom.SetData(1, data);
}

void IOCALL SubSystem::M_Set2(uint, uint data)
{
	idlecount = 0;
	piom.SetData(2, data);
}

void IOCALL SubSystem::M_SetCW(uint, uint data)
{
	idlecount = 0;
	if (data == 0x0f)
		LOG0("\ncmd: ");
	if (data & 0x80)
		cw_m = data;
	piom.SetCW(data);
}

uint IOCALL SubSystem::M_Read0(uint)
{
	idlecount = 0;
	uint d = piom.Read0();
	LOG1(">%.2x ", d);
	return d;
}

uint IOCALL SubSystem::M_Read1(uint)
{
	idlecount = 0;
	uint d = piom.Read1();
	LOG1(")%.2x ", d);
	return d;
}

uint IOCALL SubSystem::M_Read2(uint)
{
	statusdisplay.WaitSubSys();
	idlecount = 0;
	return piom.Read2();
}

// ---------------------------------------------------------------------------
//	Sub 側 PIO
//
void IOCALL SubSystem::S_Set0(uint, uint data)
{
	idlecount = 0;
//	LOG1("<a %.2x> ", data);
	pios.SetData(0, data);
}

void IOCALL SubSystem::S_Set1(uint, uint data)
{
	idlecount = 0;
//	LOG1("<b %.2x> ", data);
	pios.SetData(1, data);
}

void IOCALL SubSystem::S_Set2(uint, uint data)
{
	idlecount = 0;
//	LOG1("<c %.2x> ", data);
	pios.SetData(2, data);
}

void IOCALL SubSystem::S_SetCW(uint, uint data)
{
	idlecount = 0;
	if (data & 0x80)
		cw_s = data;
	pios.SetCW(data);
}

uint IOCALL SubSystem::S_Read0(uint)
{
	idlecount = 0;
	uint d = pios.Read0();
//	LOG1("(a %.2x) ", d);
	return d;
}

uint IOCALL SubSystem::S_Read1(uint)
{
	idlecount = 0;
	uint d = pios.Read1();
//	LOG1("(b %.2x) ", d);
	return d;
}

uint IOCALL SubSystem::S_Read2(uint)
{
	idlecount++;
	uint d = pios.Read2();
//	LOG1("(c %.2x) ", d);
	return d;
}


bool SubSystem::IsBusy()
{
	if (idlecount >= 200)
	{
		idlecount = 200;
		return false;
	}
	statusdisplay.WaitSubSys();
	return true;
}

// ---------------------------------------------------------------------------
//	状態保存
//
uint IFCALL SubSystem::GetStatusSize()
{
	return sizeof(Status);
}

bool IFCALL SubSystem::SaveStatus(uint8* s)
{
	Status* st = (Status*) s;
	st->rev = ssrev;
	for (int i=0; i<3; i++)
	{
		st->pm[i] = (uint8) piom.Port(i);
		st->ps[i] = (uint8) pios.Port(i);
	}
	st->cm = cw_m;
	st->cs = cw_s;
	st->idlecount = idlecount;
	memcpy(st->ram, ram, 0x4000);

	LOG0("\n=== SaveStatus\n");
	return true;
}

bool IFCALL SubSystem::LoadStatus(const uint8* s)
{
	const Status* st = (const Status*) s;
	if (st->rev != ssrev)
		return false;
	
	M_SetCW(0, st->cm);
	S_SetCW(0, st->cs);
	
	for (int i=0; i<3; i++)
		piom.SetData(i, st->pm[i]), pios.SetData(i, st->ps[i]);
	
	idlecount = st->idlecount;
	memcpy(ram, st->ram, 0x4000);
	LOG0("\n=== LoadStatus\n");
	return true;
}

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor SubSystem::descriptor = { indef, outdef };

const Device::InFuncPtr SubSystem::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, &IntAck),
	STATIC_CAST(Device::InFuncPtr, &M_Read0),
	STATIC_CAST(Device::InFuncPtr, &M_Read1),
	STATIC_CAST(Device::InFuncPtr, &M_Read2),
	STATIC_CAST(Device::InFuncPtr, &S_Read0),
	STATIC_CAST(Device::InFuncPtr, &S_Read1),
	STATIC_CAST(Device::InFuncPtr, &S_Read2),
};

const Device::OutFuncPtr SubSystem::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &Reset),
	STATIC_CAST(Device::OutFuncPtr, &M_Set0),
	STATIC_CAST(Device::OutFuncPtr, &M_Set1),
	STATIC_CAST(Device::OutFuncPtr, &M_Set2),
	STATIC_CAST(Device::OutFuncPtr, &M_SetCW),
	STATIC_CAST(Device::OutFuncPtr, &S_Set0),
	STATIC_CAST(Device::OutFuncPtr, &S_Set1),
	STATIC_CAST(Device::OutFuncPtr, &S_Set2),
	STATIC_CAST(Device::OutFuncPtr, &S_SetCW),
};
