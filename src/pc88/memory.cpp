// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	Main 側メモリ(含ALU)の実装
// ----------------------------------------------------------------------------
//	$Id: memory.cpp,v 1.42 2003/11/04 13:14:21 cisc Exp $

//	MemoryPage size should be equal to or less than 0x400.

#include "headers.h"
#include "file.h"
#include "device.h"
#include "device_i.h"
#include "memmgr.h"
#include "pc88/memory.h"
#include "pc88/config.h"
#include "pc88/crtc.h"
#include "error.h"
#include "status.h"

//using namespace std;
using namespace PC8801;

// ----------------------------------------------------------------------------
//	Constructor / Destructor
//
Memory::Memory(const ID& id)
  :	Device(id), rom(0), ram(0), eram(0), tvram(0), bus(0), 
	dicrom(0), cdbios(0), n80rom(0), n80v2rom(0), mm(0), mid(-1)
{
	txtwnd = 0;
	erambanks = 0;
	neweram = 4;
	waitmode = 0;
	waittype = 0;
	enablewait = false;
	for (int i=1; i<9; i++)
		erom[i] = 0;
}

Memory::~Memory()
{
	if (mm && mid != -1)
		mm->Disconnect(mid);
	delete[] rom;
	delete[] ram;
	delete[] eram;
	delete[] tvram;
	delete[] dicrom;
	delete[] cdbios;
	delete[] n80rom;
	delete[] n80v2rom;
	for (int i=1; i<9; i++)
		delete[] erom[i];
}

bool Memory::Init(MemoryManager* _mm, IOBus* _bus, CRTC* _crtc, int* wt)
{
	mm = _mm, bus = _bus, crtc = _crtc, waits = wt;
	assert(MemoryManagerBase::pagebits <= 10);
	if (MemoryManagerBase::pagebits > 10)
		return false;

	mid = mm->Connect(this);
	if (mid == -1)
		return false;

	if (!InitMemory())
		return false;
		
	port31 = port32 = port33 = port34 = port35 = 0;
	port71 = 0xff;
	port5x = 3;
	port99 = 0;
	portf0 = 0;
	port40 = 0;
	porte2 = 0;
	porte3 = 0;
	n80mode = 0;
	seldic = false;
	
	Update00R();
	Update00W();
	Update60R();
	Update80();
	UpdateC0();
	UpdateF0();
	return true;
}

// ----------------------------------------------------------------------------
//	Reset
//	
void Memory::Reset(uint, uint newmode)
{
	sw31 = bus->In(0x31);
	bool high = !(bus->In(0x6e) & 0x80);

//	port33 = 0;

	n80mode = (newmode & 2) && (port33 & 0x80 ? n80v2rom : n80rom);
	n80srmode = (newmode == Config::N80V2);

	waitmode = ((sw31 & 0x40) || (n80mode && n80srmode) ? 12 : 0) + (high ? 24 : 0);
	selgvram = true;
	port5x = 3;
	r00 = 0, r60 = 0, w00 = 0;

	// 拡張 RAM の設定
	if (n80mode)
		neweram = Max(1, neweram);
	if (erambanks != neweram)
	{
		mm->AllocR(mid, 0, 0x8000, ram);
		mm->AllocW(mid, 0, 0x8000, ram);
		
		erambanks = 0;
		delete[] eram;
		eram = new uint8[0x8000 * neweram];
		if (eram)
		{
			erambanks = neweram;
			memset(eram, 0, 0x8000*erambanks);
		}
	}
	
	mm->AllocR(mid, 0x8000, 0x8000, ram + 0x8000);
	mm->AllocW(mid, 0x8000, 0x8000, ram + 0x8000);
	if (!n80mode)
	{
		Update00R();
		Update00W();
		Update60R();
		Update80();
		UpdateC0();
		UpdateF0();
	}
	else
	{
		porte3 = 0;
		port31 = 0;
		port71 = 0xff;
		UpdateN80W();
		UpdateN80R();
		UpdateN80G();
	}
}

// ----------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e4
//	00-5f(r)*                       *  *
//	60-7f(r)*  *              *     *  *
//	00-7f(w)                        *  *
//	80-83	*              *     *      
//	84-bf                               
//	c0-ef	   *  *  *  *               
//	f0-ff	   *  *  *  *               
//	
//

//	ram     text    gvram   gvramh
//	1	2	1	2	1	42	3	4	4MHz v1
//	0	0	0.5	0.5	1	2	1	2        v2
//	1	2	1	2	7	72	4	8	8MHz
//	1	1	2	2	3.5	5	4	5

//	Wait bank	4-s		4-h		8-s		8-h
//	disp		on	off	on	off	on	off	on	off
//	normal/dma
//	00-bf		2	1	0	0	1	1	1	1
//	c0-ef		2	1	0	0	1	1	1	1
//	f0-ff		2	1	1	0	2	1	2	1
//	
//	normal/nodma
//	00-bf		1   1   0   0   1   1   1   1
//	c0-ef		1   1   0   0   1   1   1   1
//	f0-ff		1   1   0   0   1   1   1   1
//
//	gvram
//	00-bf		2	1	0	0	1	1	1	1
//	c0-ef		42	1	2	1	72	7	5	3
//	f0-ff		42	1	2	1	72	7	5	3
//
//	00-bf		1	1	0	0	1	1	1	1
//	c0-ef		42	1	2	1	72	7	5	3
//	f0-ff		42	1	2	1	72	7	5	3
//
//	gvram(h)
//	00-bf		2	1	0	0	1	1	1	1
//	c0-ef		4	3	2	1	8	4	5	3
//	f0-ff		4	3	2	1	8	4	5	3
//
//	00-bf		1	1	0	0	1	1	1	1
//	c0-ef		4	3	2	1	8	4	5	3
//	f0-ff		4	3	2	1	8	4	5	3

const Memory::WaitDesc Memory::waittable[48] =
{	
	{ 2, 2, 2},{ 1, 1, 1}, 	{ 1, 1, 1},{ 1, 1, 1}, 
	{30,30,30},{ 3, 3, 3},  {30,30,30},{ 3, 3, 3}, 	 
	{ 4, 4, 4},{ 3, 3, 3},	{ 4, 4, 4},{ 3, 3, 3},

	{ 0, 0, 1},{ 0, 0, 0}, 	{ 0, 0, 0},{ 0, 0, 0}, 
	{ 0, 2, 2},{ 0, 1, 1}, 	{ 0, 2, 2},{ 0, 1, 1}, 
	{ 0, 2, 2},{ 0, 1, 1},	{ 0, 2, 2},{ 0, 1, 1},

	{ 1, 1, 2},{ 1, 1, 1}, 	{ 1, 1, 1},{ 1, 1, 1}, 
	{72,72,72},{ 1, 7, 7}, 	{72,72,72},{ 1, 7, 7}, 
	{ 1, 8, 8},{ 1, 4, 4},	{ 1, 8, 8},{ 1, 4, 4},
	
	{ 1, 1, 2},{ 1, 1, 1}, 	{ 1, 1, 1},{ 1, 1, 1}, 
	{ 1, 5, 5},{ 1, 3, 3}, 	{ 1, 5, 5},{ 1, 3, 3}, 
	{ 1, 5, 5},{ 1, 3, 3},	{ 1, 5, 5},{ 1, 3, 3},
};

// ----------------------------------------------------------------------------
//	Port31
//	b2 b1
//	0  0	N88
//	1  0	N80
//	x  1	RAM
//	
void IOCALL Memory::Out31(uint, uint data)
{
	if (!n80mode)
	{
		if ((data ^ port31) & 6)
		{
			port31 = data & 6;
			Update00R();
			Update60R();
			Update80();
		}
	}
	else
	{
		if ((data ^ port31) & 3)
		{
			port31 = data & 3;
			UpdateN80R();
			UpdateN80W();
		}
	}
}

// ----------------------------------------------------------------------------
//	Port32
//	b5		ALU Enable (port5x=RAM)
//	b4		RAM/~TVRAM (V1S 無効)
//	b1 b0	N88 EROM bank select
//	
void IOCALL Memory::Out32(uint, uint data)
{
	if (!n80mode)
	{
		uint mod = data ^ port32;
		port32 = data;
		if (mod & 0x03)
			Update60R();
		if (mod & 0x40)
			UpdateC0();
		if (mod & 0x50)
			UpdateF0();
	}
}

// ----------------------------------------------------------------------------
//	Port33
//	b7		0...N/N80mode   1...N80V2mode
//	b6		ALU Enable (port5x=RAM)
//	
void IOCALL Memory::Out33(uint, uint data)
{
	if (n80mode)
	{
		uint mod = data ^ port33;
		port33 = data;
		if (mod & 0x80) 
			UpdateN80R();
		if (mod & 0x40)
			UpdateN80G();
	}
}

// ----------------------------------------------------------------------------
//	Port34	ALU Operation
//
void IOCALL Memory::Out34(uint, uint data)
{
	if (!n80mode || n80srmode)
	{
		port34 = data;
		for (uint i=0; i<3; i++)
		{
			switch (data & 0x11)
			{
			case 0x00: maskr.byte[i] = 0xff; masks.byte[i] = 0x00; maski.byte[i] = 0x00; break;
			case 0x01: maskr.byte[i] = 0x00; masks.byte[i] = 0xff; maski.byte[i] = 0x00; break;
			case 0x10: maskr.byte[i] = 0x00; masks.byte[i] = 0x00; maski.byte[i] = 0xff; break;
			case 0x11: maskr.byte[i] = 0x00; masks.byte[i] = 0x00; maski.byte[i] = 0x00; break;
			}
			data >>= 1;
		}
	}
}

// ----------------------------------------------------------------------------
//	Port35	
//	b7		ALU Enable?
//	b5-b4	ALU Write Control
//	b2-b0	ALU Read MUX
//
void IOCALL Memory::Out35(uint, uint data)
{
	if (!n80mode || n80srmode)
	{
		port35 = data;
		aluread.byte[0] = (data & 1) ? 0xff : 0x00;
		aluread.byte[1] = (data & 2) ? 0xff : 0x00;
		aluread.byte[2] = (data & 4) ? 0xff : 0x00;
		
		if (data & 0x80)
		{
			port5x = 3;
		}
		if (!n80mode)
		{
			UpdateC0();
			UpdateF0();
		}
		else
		{
			UpdateN80G();
		}
	}
}

// ----------------------------------------------------------------------------
//	Port5c	GVRAM0
//	port5d	GVRAM1
//	port5e	GVRAM2
//	port5f	RAM
//
void IOCALL Memory::Out5x(uint bank, uint)
{
	bank &= 3;
	if (n80mode)
	{
		if (!n80srmode)
		{
			if (bank == 1 || bank == 2)
				return;
		}
		port5x = bank;
		UpdateN80G();
	}
	else
	{
		port5x = bank;
		UpdateC0();
		UpdateF0();
	}
}

// ----------------------------------------------------------------------------
//	Port70	TextWindow (N88 時有効)
//
void IOCALL Memory::Out70(uint, uint data)
{
	if (!n80mode)		// 80SR では port70 が hold されないってことかな？
	{
		txtwnd = data * 0x100;
		if ((port31 & 6) == 0)
		{
			Update80();
		}
	}
}

// ----------------------------------------------------------------------------
//	Port71
//	b0		N88ROM/~EROM
//
void IOCALL Memory::Out71(uint, uint data)
{
	port71 = (data | erommask) & 0xff;
	if (!n80mode)
	{
		if ((port31 & 6) == 0)
		{
//			Update00R();
			Update60R();
		}
	}
	else if (port33 & 0x80)
	{
		UpdateN80R();
	}
}

// ----------------------------------------------------------------------------
//	Port78	++Port70
//
void IOCALL Memory::Out78(uint, uint)
{
	txtwnd = (txtwnd + 0x100) & 0xff00;
	if ((port31 & 6) == 0)
	{
		Update80();
	}
}

// ----------------------------------------------------------------------------
//	Port99
//	b4		CD-BIOS/other
//	b0		CD-EROM
//
void IOCALL Memory::Out99(uint, uint data)
{
	if (cdbios && !n80mode)
	{
		port99 = data & 0x11;
		Update00R();
		Update60R();
	}
	else
	{
		port99 = 0;
	}
}

// ----------------------------------------------------------------------------
//	Porte2	
//	b4		erom write enable (pe4 < ERAM Pages)
//	b0		erom read enable
//
void IOCALL Memory::Oute2(uint, uint data)
{
	porte2 = data;
	if (!n80mode)
	{
		Update00R();
		Update60R();
		Update00W();
	}
	else
	{
		UpdateN80R();
		UpdateN80W();
	}
}

// ----------------------------------------------------------------------------
//	Porte4	erom page select
//
void IOCALL Memory::Oute3(uint, uint data)
{
	porte3 = data;
	if (!n80mode)
	{
		Update00R();
		Update60R();
		Update00W();
	}
	else
	{
		UpdateN80R();
		UpdateN80W();
	}
}

// ----------------------------------------------------------------------------
//	Port F0	辞書ROMバンク選択
//	
void IOCALL Memory::Outf0(uint, uint data)
{
	portf0 = data;
	if (dicrom && !n80mode)
	{
		UpdateC0();
		UpdateF0();
	}
}

// ----------------------------------------------------------------------------
//	Port F0	辞書ROMバンク選択
//	
void IOCALL Memory::Outf1(uint, uint data)
{
	if (dicrom && !n80mode)
	{
		seldic = !(data & 1);
		UpdateC0();
		UpdateF0();
	}
}

// ----------------------------------------------------------------------------
//	In xx
//
uint IOCALL Memory::In32(uint)
{
	return port32;
}

uint IOCALL Memory::In33(uint)
{
	return n80mode ? port33 : 0xff;
}

uint IOCALL Memory::In5c(uint)
{
	static const uint res[4] = { 0xf9, 0xfa, 0xfc, 0xf8 };
	return res[port5x & 3];	
}

uint IOCALL Memory::In70(uint)
{
	return txtwnd >> 8;
}

uint IOCALL Memory::In71(uint)
{
	return port71;
}

uint IOCALL Memory::Ine2(uint)
{
	return (~porte2) | 0xee;
}

uint IOCALL Memory::Ine3(uint)
{
	return porte3;
}

// ----------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e3
//	00-5f(r)*                       *  *  *  *
//
void Memory::Update00R()
{
	uint8* read;

	if ((porte2 & 0x01) && (porte3 < erambanks))
	{
		read = &eram[porte3 * 0x8000];
	}
	else
	{
		read = ram;

		if (!(port31 & 2))
		{
			// ROM
			if (port99 & 0x10)
			{
				read = cdbios + (port31 & 4 ? 0x8000 : 0);
			}
			else
			{
				read = rom + (port31 & 4 ? n80 : n88);
			}
		}
	}
	if (r00 != read)
	{
		r00 = read;
		mm->AllocR(mid, 0, 0x6000, read);
	}
}

// ----------------------------------------------------------------------------
//	00-7f(r)
//
void Memory::UpdateN80R()
{
	uint8* read;
	uint8* read60;

	if (((porte2 | (port31 >> 1)) & 1) && porte3 < erambanks)
	{
		read = &eram[porte3 * 0x8000];
		read60 = read+0x6000;
	}
	else
	{
		if (port33 & 0x80)
		{
			read = n80v2rom;
			read60 = read + (port71 & 1 ? 0x6000 : 0x8000);
		}
		else
		{
			read = n80rom;
			read60 = ((port31 | (erommask >> 8)) & 1) ? read+0x6000 : erom[8];
		}
	}	
	if (r00 != read)
	{
		r00 = read;
		mm->AllocR(mid, 0, 0x6000, read);
	}
	if (r60 != read60)
	{
		r60 = read60;
		mm->AllocR(mid, 0x6000, 0x2000, read60);
	}
}

// ----------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e3
//	60-7f(r)*  *              *     *  *  *  *
//
void Memory::Update60R()
{
	uint8* read;
	if ((porte2 & 0x01) && (porte3 < erambanks))
	{
		read = &eram[porte3 * 0x8000] + 0x6000;
	}
	else
	{
		read = ram + 0x6000;
		
		if ((port31 & 6) == 0)
		{
			if (port99 & 0x10)
				read = cdbios + 0x6000;
			else
			{
				if (port71 == 0xff)
				{
					read = rom + n88 + 0x6000;
				}
				else
				{
					if (port71 & 1)
					{
						for (int i=7; i>0; i--)
						{
							if (~port71 & (1 << i))
							{
								read = erom[i];
								break;
							}
						}
					}
					else
						read = rom + n88e + 0x2000 * (port32 & 3);
				}
			}
		}
		else if ((port31 & 6) == 4)
		{
			if (port99 & 0x10)
				read = cdbios + 0x8000 + 0x6000;
			else
				read = rom + n80 + 0x6000;
		}
	}
	if (r60 != read)
	{
		r60 = read;
		mm->AllocR(mid, 0x6000, 0x2000, read);
	}
}

// ---------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e4
//	00-7f(w)                        *  *
//
void Memory::Update00W()
{
	uint8* write;
 
	if ((porte2 & 0x10) && (porte3 < erambanks))
	{
		write = &eram[porte3 * 0x8000];
	}
	else
	{
		write = ram;
	}

	if (w00 != write)
	{
		w00 = write;
		mm->AllocW(mid, 0, 0x8000, write);
	}
}

// ---------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e4
//	00-7f(w)   *                    *  *
//
void Memory::UpdateN80W()
{
	uint8* write;
 
	if (((porte2 & 0x10) || (port31 & 0x02)) && (porte3 < erambanks))
	{
		write = &eram[porte3 * 0x8000];
	}
	else
	{
		write = ram;
	}

	if (w00 != write)
	{
		w00 = write;
		mm->AllocW(mid, 0, 0x8000, write);
	}
}

// ---------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e4
//	80-83	*              *     *      
//
void Memory::Update80()
{
	if ((port31 & 6) != 0)
	{
		mm->AllocR(mid, 0x8000, 0x400, ram + 0x8000);
		mm->AllocW(mid, 0x8000, 0x400, ram + 0x8000);
	}
	else
	{
		if (txtwnd <= 0xfc00)
		{
			mm->AllocR(mid, 0x8000, 0x400, ram + txtwnd);
			mm->AllocW(mid, 0x8000, 0x400, ram + txtwnd);
		}
		else
		{
			mm->AllocR(mid, 0x8000, 0x400, RdWindow);
			mm->AllocW(mid, 0x8000, 0x400, WrWindow);
		}
	}
}

// ----------------------------------------------------------------------------
//	テキストウィンドウ(ラップアラウンド)のアクセス
//
void MEMCALL Memory::WrWindow(void* inst, uint addr, uint data)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	m->ram[(m->txtwnd + (addr & 0x3ff)) & 0xffff] = data;
}

uint MEMCALL Memory::RdWindow(void* inst, uint addr)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	return m->ram[(m->txtwnd + (addr & 0x3ff)) & 0xffff];
}


void Memory::SelectJisyo()
{
	if (seldic)
	{
		uint8* mem = dicrom + (portf0 & 0x1f) * 0x4000;
		if (mem != rc0)
		{
			rc0 = mem;
			mm->AllocR(mid, 0xc000, 0x4000, rc0);
		}
	}
}

// ----------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e4
//	c0-ef	   *  *  *  *               
//
void Memory::UpdateC0()
{
	// is gvram selected ? 
	if (port32 & 0x40)
	{
		port5x = 3;
		if (port35 & 0x80)
		{
			rc0 = 0;
			SelectALU(0xc000);
			SelectJisyo();
			return;
		}
	}
	else
	{
		if (port5x < 3)
		{
			rc0 = 0;
			SelectGVRAM(0xc000);
			SelectJisyo();
			return;
		}
	}

	// Normal RAM ? 
	if (selgvram)
	{
		selgvram = false;
		rc0 = ram + 0xc000;
		mm->AllocR(mid, 0xc000, 0x3000, ram + 0xc000);
		mm->AllocW(mid, 0xc000, 0x3000, ram + 0xc000);
		waittype &= 3;
		SetWait();
	}

	if (seldic)
	{
		SelectJisyo();
		return;
	}

	if (rc0 != ram + 0xc000)
	{
		rc0 = ram + 0xc000;
		mm->AllocR(mid, 0xc000, 0x3000, ram + 0xc000);
	}
}

// ----------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e4
//	f0-ff	   *  *  *  *               
//
void Memory::UpdateF0()
{
	if (!selgvram && !seldic)
	{
		uint8* mem;
		
		if (!(port32 & 0x10) && (sw31 & 0x40))
			mem = tvram;
		else
			mem = ram + 0xf000;

		mm->AllocR(mid, 0xf000, 0x1000, mem);
		mm->AllocW(mid, 0xf000, 0x1000, mem);
	}
}

// ----------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e4
//	80-ff	             *
//
void Memory::UpdateN80G()
{
	if ((port33 & 0x40) != 0)
	{
		// 80SR, ALU
		port5x = 3;
		if (port35 & 0x80)
		{
			rc0 = 0;
			SelectALU(0x8000);
			return;
		}
	}
	else
	{
		if (port5x < 3)
		{
			rc0 = 0;
			SelectGVRAM(0x8000);
			return;
		}
	}
	// Normal RAM ? 
	if (selgvram)
	{
		selgvram = false;
		mm->AllocR(mid, 0x8000, 0x4000, ram+0x8000);
		mm->AllocW(mid, 0x8000, 0x4000, ram+0x8000);
		waittype &= 3;
		SetWait();
	}
}

// ----------------------------------------------------------------------------
//	Select GVRAM
//	port 5c-5e
//
void Memory::SelectGVRAM(uint gvtop)
{
	static const MemoryManager::RdFunc rf[3] =
	{
		RdGVRAM0, RdGVRAM1, RdGVRAM2
	};
	mm->AllocR(mid, gvtop, 0x4000, rf[port5x & 3]);

	static const MemoryManager::WrFunc wf[3] =
	{
		WrGVRAM0, WrGVRAM1, WrGVRAM2
	};
	mm->AllocW(mid, gvtop, 0x4000, wf[port5x & 3]);

	if (!selgvram)
	{
		selgvram = true;
		waittype = (waittype & 3) | (port40 & 0x10 ? 8 : 4);
		SetWait();
	}
}

// ----------------------------------------------------------------------------
//	GVRAM の読み書き
//
#define SETDIRTY(addr)	\
	if (m->dirty[addr >> 4]) return; \
	else m->dirty[addr >> 4] = 1;

void MEMCALL Memory::WrGVRAM0(void* inst, uint addr, uint data)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	m->gvram[addr &= 0x3fff].byte[0] = data;
	SETDIRTY(addr);
}

void MEMCALL Memory::WrGVRAM1(void* inst, uint addr, uint data)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	m->gvram[addr &= 0x3fff].byte[1] = data;
	SETDIRTY(addr);
}

void MEMCALL Memory::WrGVRAM2(void* inst, uint addr, uint data)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	m->gvram[addr &= 0x3fff].byte[2] = data;
	SETDIRTY(addr);
}

uint MEMCALL Memory::RdGVRAM0(void* inst, uint addr)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	return m->gvram[addr & 0x3fff].byte[0];
}

uint MEMCALL Memory::RdGVRAM1(void* inst, uint addr)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	return m->gvram[addr & 0x3fff].byte[1];
}

uint MEMCALL Memory::RdGVRAM2(void* inst, uint addr)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	return m->gvram[addr & 0x3fff].byte[2];
}

// ----------------------------------------------------------------------------
//	Select ALU
//
void Memory::SelectALU(uint gvtop)
{
	static const MemoryBus::WriteFuncPtr funcs[4] =
	{
		WrALUSet,	WrALURGB,	WrALUB,		WrALUR
	};

	mm->AllocR(mid, gvtop, 0x4000, RdALU);
	mm->AllocW(mid, gvtop, 0x4000, funcs[(port35 >> 4) & 3]);

	if (!selgvram)
	{
		selgvram = true;
		waittype = (waittype & 3) | (port40 & 0x10 ? 8 : 4);
		SetWait();
	}
}

// ----------------------------------------------------------------------------
//	GVRAM R/W thru ALU
//
uint MEMCALL Memory::RdALU(void* inst, uint addr)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	quadbyte q;
	m->alureg = m->gvram[addr & 0x3fff];
	q.pack = m->alureg.pack ^ m->aluread.pack;
	return ~(q.byte[0] | q.byte[1] | q.byte[2]) & 0xff;
}

void MEMCALL Memory::WrALUSet(void* inst, uint addr, uint data)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	
	quadbyte q;
//	data &= 255;
	q.pack = (data << 16) | (data << 8) | data;
	addr &= 0x3fff;
	m->gvram[addr].pack = 
		((m->gvram[addr].pack 
		& ~(q.pack & m->maskr.pack)) 
		 | (q.pack & m->masks.pack)) 
		 ^ (q.pack & m->maski.pack);
	SETDIRTY(addr);
}

void MEMCALL Memory::WrALURGB(void* inst, uint addr, uint)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	m->gvram[addr &= 0x3fff] = m->alureg;
	SETDIRTY(addr);
}

void MEMCALL Memory::WrALUR(void* inst, uint addr, uint)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	m->gvram[addr &= 0x3fff].byte[1] = m->alureg.byte[0];
	SETDIRTY(addr);
}

void MEMCALL Memory::WrALUB(void* inst, uint addr, uint)
{
	Memory* m = STATIC_CAST(Memory*, inst);
	m->gvram[addr &= 0x3fff].byte[0] = m->alureg.byte[1];
	SETDIRTY(addr);
}

// ----------------------------------------------------------------------------
//	メモリの割り当てと ROM の読み込み。
//	
bool Memory::InitMemory()
{
	delete rom;		rom   = new uint8[romsize];
	delete ram;		ram   = new uint8[0x10000];
	delete tvram;	tvram = new uint8[0x1000];
	
	if (!(rom && ram && tvram))
	{
		Error::SetError(Error::OutOfMemory);
		return false;
	}
	SetRAMPattern(ram, 0x10000);
	memset(gvram, 0, sizeof(quadbyte) * 0x4000);
	memset(tvram, 0, 0x1000);

	ram[0xff33] = 0;		// PACMAN 対策


	mm->AllocR(mid, 0, 0x10000, ram);
	mm->AllocW(mid, 0, 0x10000, ram);
	
	if (!LoadROM())
	{
		Error::SetError(Error::NoROM);
		return false;
	}
	return true;
}

// ----------------------------------------------------------------------------
//	必須でない ROM を読み込む
//	
bool Memory::LoadOptROM(const char* name, uint8*& rom, int size)
{
	FileIO file;
	if (file.Open(name, FileIO::readonly))
	{
		file.Seek(0, FileIO::begin);
		delete[] rom;
		rom = new uint8[size];
		if (rom)
		{
			int r = file.Read(rom, size);
			memset(rom + r, 0xff, size - r);
			if (r > 0)
				return true;
		}
	}
	delete[] rom;
	rom = 0;
	return false;
}

// ----------------------------------------------------------------------------
//	ROM を読み込む
//	
bool Memory::LoadROM()
{
	FileIO file;

	LoadOptROM("jisyo.rom", dicrom, 512*1024);
	LoadOptROM("cdbios.rom", cdbios, 0x10000);
	LoadOptROM("n80_2.rom", n80rom, 0x8000);
	LoadOptROM("n80_3.rom", n80v2rom, 0xa000);
	char name[] = "e0.rom";
	
	erommask = ~1;
	for (int i=1; i<9; i++)
	{
		name[1] = '0'+i;
		if (LoadOptROM(name, erom[i], 0x2000))
			erommask &= ~(1 << i);
	}
	
	if (file.Open("pc88.rom", FileIO::readonly))
	{
		file.Seek(0, FileIO::begin);
		file.Read(rom + n88, 0x8000);
		file.Read(rom + n80 + 0x6000, 0x2000);
		file.Seek(0x2000, FileIO::current);
		file.Read(rom + n88e, 0x8000);
		file.Seek(0x2000, FileIO::current);
		file.Read(rom + n80, 0x6000);
		return true;
	}
	
	if (!LoadROMImage(rom+n88, "n88.rom", 0x8000))
		return false;
	LoadROMImage(rom+n80,         "n80.rom",   0x8000);
	LoadROMImage(rom+n88e,        "n88_0.rom", 0x2000);
	LoadROMImage(rom+n88e+0x2000, "n88_1.rom", 0x2000);
	LoadROMImage(rom+n88e+0x4000, "n88_2.rom", 0x2000);
	LoadROMImage(rom+n88e+0x6000, "n88_3.rom", 0x2000);
	
	return true;
}

bool Memory::LoadROMImage(uint8* dest, const char* filename, int size)
{
	FileIO file;
	if (!file.Open(filename, FileIO::readonly))
		return false;
	file.Read(dest, size);
	return true;
}

// ----------------------------------------------------------------------------
//	起動時メモリパターン作成 (SR 型)
//	arg:	ram		RAM エリア
//			length	RAM エリア長 (0x80 の倍数)
//
void Memory::SetRAMPattern(uint8* ram, uint length)
{
	if (length > 0)
	{
		for (uint i=0; i<length; i+=0x80, ram+=0x80)
		{
			uint8 b = ((i >> 7) ^ i) & 0x100 ? 0x00 : 0xff;
			memset(ram,       b, 0x40);
			memset(ram+0x40, ~b, 0x40);
			ram[0x7f] = b;
		}
		ram[-1] = 0;
	}
}

// ----------------------------------------------------------------------------
//	ウェイト情報の更新
//	
void Memory::SetWait()
{
	if (enablewait)
	{
		const WaitDesc& wait = waittable[waitmode + waittype];
		if (!n80mode)
		{
			SetWaits(0x0000, 0xc000, wait.b0);
			SetWaits(0xc000, 0x3000, wait.bc);
			SetWaits(0xf000, 0x1000, wait.bf);
		}
		else
		{
			SetWaits(0x0000, 0x8000, wait.b0);
			SetWaits(0x8000, 0x4000, wait.bc);
			SetWaits(0xc000, 0x4000, wait.b0);
		}
	}
}

// ----------------------------------------------------------------------------
//	VRTC(ウェイト変更)
//	
void Memory::VRTC(uint, uint d)
{
	waittype = (waittype & ~3) | (d & 1) | (crtc->GetStatus() & 0x10 ? 0 : 2);
	SetWait();
}

// ----------------------------------------------------------------------------
//	Out40
//	
void IOCALL Memory::Out40(uint, uint data)
{
	data &= 0x10;
	if (data ^ port40)
	{
		port40 = data;
		if (selgvram)
		{
			waittype = (waittype & 3) | (port40 & 0x10 ? 8 : 4);
			SetWait();
		}
	}
}

// ---------------------------------------------------------------------------
//	設定反映
//
void Memory::ApplyConfig(const Config* cfg)
{
	enablewait = (cfg->flags & Config::enablewait) != 0;
	neweram = cfg->erambanks;
	if (enablewait)
		SetWait();
	else
		SetWaits(0, 0x10000, 0);
}

// ---------------------------------------------------------------------------

inline void Memory::SetWaits(uint a, uint s, uint v)
{
	if (waits)
	{
		uint p = a >> MemoryManager::pagebits;
		uint t = (a + s + MemoryManager::pagemask) >> MemoryManager::pagebits;
		for (; p<t; p++)
			waits[p] = v;
	}
}

// ---------------------------------------------------------------------------
//	状態保存
//
uint IFCALL Memory::GetStatusSize()
{
	return sizeof(Status) + erambanks * 0x8000 - 1;
}

bool IFCALL Memory::SaveStatus(uint8* s)
{
	Status* status = (Status*) s;
	status->rev = ssrev;
	status->p31 = uint8(port31);
	status->p32 = uint8(port32);
	status->p33 = uint8(port33);
	status->p34 = uint8(port34);
	status->p35 = uint8(port35);
	status->p40 = uint8(port40);
	status->p5x = uint8(port5x);
	status->p70 = uint8(In70(0));
	status->p71 = uint8(port71);
	status->p99 = uint8(port99);
	status->pe2 = uint8(porte2);
	status->pe3 = uint8(porte3);
	status->pf0 = uint8(portf0);

	memcpy(status->ram, ram, 0x10000);
	memcpy(status->tvram, tvram, 0x1000);
	for (int i=0; i<3; i++)
		for (int j=0; j<0x4000; j++)
			status->gvram[i][j] = gvram[j].byte[i]; 
	memcpy(status->eram, eram, 0x8000 * erambanks);
	return true;
}

bool IFCALL Memory::LoadStatus(const uint8* s)
{
	const Status* status = (const Status*) s;
	if (status->rev != ssrev)
		return false;
	
	Out34(0, status->p34);
	Out31(0, status->p31);
	Out32(0, status->p32);
	Out33(0, status->p33);
	Out35(0, status->p35);
	Out5x(status->p5x, 0);
	Out40(0, status->p40);
	Out71(0, status->p71);
	Out70(0, status->p70);
	Out99(0, status->p99);
	Oute2(0, status->pe2);
	Oute3(0, status->pe3);
	Outf0(0, status->pf0);
	
	memcpy(ram, status->ram, 0x10000);
	memcpy(tvram, status->tvram, 0x1000);
	for (int i=0; i<3; i++)
		for (int j=0; j<0x4000; j++)
			gvram[j].byte[i] = status->gvram[i][j]; 
	memset(dirty, 1, 0x400);
	memcpy(eram, status->eram, 0x8000 * erambanks);
	return true;
}


// ---------------------------------------------------------------------------
//	Device descriptor
//	
const Device::Descriptor Memory::descriptor =
{
	Memory::indef, Memory::outdef
};

const Device::OutFuncPtr Memory::outdef[] =
{
	STATIC_CAST(Device::OutFuncPtr, &Reset),
	STATIC_CAST(Device::OutFuncPtr, &Out31),
	STATIC_CAST(Device::OutFuncPtr, &Out32),
	STATIC_CAST(Device::OutFuncPtr, &Out34),
	STATIC_CAST(Device::OutFuncPtr, &Out35),
	STATIC_CAST(Device::OutFuncPtr, &Out40),
	STATIC_CAST(Device::OutFuncPtr, &Out5x),
	STATIC_CAST(Device::OutFuncPtr, &Out70),
	STATIC_CAST(Device::OutFuncPtr, &Out71),
	STATIC_CAST(Device::OutFuncPtr, &Out78),
	STATIC_CAST(Device::OutFuncPtr, &Out99),
	STATIC_CAST(Device::OutFuncPtr, &Oute2),
	STATIC_CAST(Device::OutFuncPtr, &Oute3),
	STATIC_CAST(Device::OutFuncPtr, &Outf0),
	STATIC_CAST(Device::OutFuncPtr, &Outf1),
	STATIC_CAST(Device::OutFuncPtr, &VRTC),
	STATIC_CAST(Device::OutFuncPtr, &Out33),
};

const Device::InFuncPtr Memory::indef[] =
{
	STATIC_CAST(Device::InFuncPtr, &In32),
	STATIC_CAST(Device::InFuncPtr, &In5c),
	STATIC_CAST(Device::InFuncPtr, &In70),
	STATIC_CAST(Device::InFuncPtr, &In71),
	STATIC_CAST(Device::InFuncPtr, &Ine2),
	STATIC_CAST(Device::InFuncPtr, &Ine3),
	STATIC_CAST(Device::InFuncPtr, &In33),
};



// ----------------------------------------------------------------------------
//			31 32 34 35 5x 70 71 78 e2 e3
//	00-5f(r)*                       *  *  *  *
//
inline uint Memory::GetHiBank(uint addr)
{
	if (port32 & 0x40)
	{
		if (port35 & 0x80)
			return mALU;
	}
	else
	{
		if (port5x < 3)
			return mG0 + port5x;
	}

	if (addr < 0xf000)
		return mRAM;

	if (!(port32 & 0x10) && (sw31 & 0x40))
		return mTV;
	return mRAM;
}

uint IFCALL Memory::GetRdBank(uint addr)
{
	if (addr < 0x8000)
	{
		if ((porte2 & 0x01) && (porte3 < erambanks))
			return mERAM + porte3;
		else
		{
			if (port31 & 2)
				return mRAM;
			else
			{
				if (port99 & 0x10)
					return port31 & 4 ? mCD1 : mCD0;
				else if (port31 & 4)
					return mN;
				else if (addr < 0x6000 || port71 == 0xff)
					return mN88;

				if (port71 & 1)
				{
					for (int i=6; i>=0; i--)
					{
						if (~port71 & (2 << i))
							return mE1 + i;
					}
				}
				else
					return mN88E0 + (port32 & 3);
			}
		}
	}

	if (addr < 0xc000)
		return mRAM;

	if (seldic)
		return mJISYO;

	return GetHiBank(addr);
}

uint IFCALL Memory::GetWrBank(uint addr)
{
	if (addr < 0x8000)
	{
		if ((porte2 & 0x10) && (porte3 < erambanks))
			return mERAM + porte3;
		return mRAM;
	}
	if (addr < 0xc000)
		return mRAM;
	return GetHiBank(addr);
}

