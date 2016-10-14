// ---------------------------------------------------------------------------
//	PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	デバイスと進行管理
// ---------------------------------------------------------------------------
//	$Id: pc88.cpp,v 1.53 2003/09/28 14:35:35 cisc Exp $

//	Memory Bus Banksize <= 0x400

#include "headers.h"
#include "pc88/pc88.h"
#include "pc88/config.h"
#include "pc88/memory.h"
#include "pc88/pd8257.h"
#include "pc88/kanjirom.h"
#include "pc88/screen.h"
#include "pc88/intc.h"
#include "pc88/crtc.h"
#include "pc88/base.h"
#include "pc88/fdc.h"
#include "pc88/subsys.h"
#include "pc88/sio.h"
#include "pc88/opnif.h"
#include "pc88/diskmgr.h"
#include "pc88/tapemgr.h"
#include "pc88/beep.h"
#include "pc88/joypad.h"
#include "calender.h"
#include "loadmon.h"

#include "status.h"
#include "device_i.h"

//#define LOGNAME "pc88"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築・破棄
//
PC88::PC88()
  :	cpu1(DEV_ID('C', 'P', 'U', '1')), cpu2(DEV_ID('C', 'P', 'U', '2')),	
	base(0), mem1(0), dmac(0), 	knj1(0), knj2(0), scrn(0), intc(0), crtc(0), 
	fdc (0), subsys(0), siotape(0), opn1(0), opn2(0), caln(0), diskmgr(0),
	beep(0), siomidi(0), joypad(0)
{
	assert((1 << MemoryManager::pagebits) <= 0x400); 
	clock = 100;
	DIAGINIT(&cpu1);
	dexc = 0;
}

PC88::~PC88()
{
//	devlist.Cleanup();

	delete base;
	delete mem1;
	delete dmac;
	delete knj1;
	delete knj2;
	delete scrn;
	delete intc;
	delete crtc;
	delete fdc;
	delete subsys;
	delete siotape;
	delete siomidi;
	delete caln;
	delete joypad;
}


// ---------------------------------------------------------------------------
//	初期化
//
bool PC88::Init(Draw* _draw, DiskManager* disk, TapeManager* tape)
{
	draw = _draw;
	diskmgr = disk;
	tapemgr = tape;
	
	if (!Scheduler::Init())
		return false;
	
	if (!draw->Init(640, 400, 8))
		return false;
	
	if (!tapemgr->Init(this, 0, 0))
		return false;

	MemoryPage* read, * write;

	cpu1.GetPages(&read, &write);
	if (!mm1.Init(0x10000, read, write))
		return false;
	
	cpu2.GetPages(&read, &write);
	if (!mm2.Init(0x10000, read, write))
		return false;

	if (!bus1.Init(portend, &devlist) || !bus2.Init(portend2, &devlist))
		return false;

	if (!ConnectDevices() || !ConnectDevices2()) 
		return false;

	Reset();
	region.Reset();
	clock = 1;
	return true;
}

void PC88::DeInit()
{
	delete beep;
	delete opn1;
	delete opn2;
}

// ---------------------------------------------------------------------------
//	執行
//	1 tick = 10μs
//
int PC88::Proceed(uint ticks, uint clk, uint ecl)
{
	clock = Max(1, clk);
	eclock = Max(1, ecl);
	return Scheduler::Proceed(ticks);
}

// ---------------------------------------------------------------------------
//	実行
//
int PC88::Execute(int ticks)
{
	LOADBEGIN("Core.CPU");
	int exc = ticks * clock;
	if (!(cpumode & stopwhenidle) || subsys->IsBusy() || fdc->IsBusy())
	{
		if ((cpumode & 1) == ms11)
			exc = Z80::ExecDual(&cpu1, &cpu2, exc);
		else
			exc = Z80::ExecDual2(&cpu1, &cpu2, exc);
	}
	else
	{
		exc = Z80::ExecSingle(&cpu1, &cpu2, exc);
	}
	exc += dexc;
	dexc = exc % clock;
	LOADEND("Core.CPU");
	return exc / clock;
}

// ---------------------------------------------------------------------------
//	実行クロック数変更
//
void PC88::Shorten(int ticks)
{
	Z80::StopDual(ticks * clock);
}

int PC88::GetTicks()
{
	return (Z80::GetCCount() + dexc) / clock;
}

// ---------------------------------------------------------------------------
//	VSync
//
void PC88::VSync()
{
	statusdisplay.UpdateDisplay();
	if (cfgflags & Config::watchregister)
		statusdisplay.Show(10, 0, "%.4X(%.2X)/%.4X", cpu1.GetPC(), cpu1.GetReg().ireg, cpu2.GetPC());
}

// ---------------------------------------------------------------------------
//	画面更新
//
void PC88::UpdateScreen(bool refresh)
{
	int dstat = draw->GetStatus();
	if (dstat & Draw::shouldrefresh)
		refresh = true;

	LOADBEGIN("Screen");

	if (!updated || refresh)
	{
		if (!(cfgflags & Config::drawprioritylow) || (dstat & (Draw::readytodraw | Draw::shouldrefresh)))
//		if (dstat & (Draw::readytodraw | Draw::shouldrefresh))
		{
			int	bpl;
			uint8* image;

//			crtc->SetSize();
			if (draw->Lock(&image, &bpl))
			{
				LOG2("(%d -> %d) ", region.top, region.bottom);
				crtc->UpdateScreen(image, bpl, region, refresh);
				LOG2("(%d -> %d) ", region.top, region.bottom);
				scrn->UpdateScreen(image, bpl, region, refresh);
				LOG2("(%d -> %d)\n", region.top, region.bottom);
				
				bool palchanged = scrn->UpdatePalette(draw);
				draw->Unlock();
				updated = palchanged || region.Valid();
			}
		}
	}
	LOADEND("Screen");
	if (draw->GetStatus() & Draw::readytodraw)
	{
		if (updated)
		{
			updated = false;
			draw->DrawScreen(region);
			region.Reset();
		}
		else
		{
			Draw::Region r;
			r.Reset();
			draw->DrawScreen(r);
		}
	}
}

// ---------------------------------------------------------------------------
//	リセット
//
void PC88::Reset()
{
	bool cd = false;
	if (IsCDSupported())
		cd = (base->GetBasicMode() & 0x40) != 0;

	base->SetFDBoot(cd || diskmgr->GetCurrentDisk(0) >= 0);
	base->Reset();		// Switch 関係の更新

	bool isv2 = (bus1.In(0x31) & 0x40) != 0;
	bool isn80v2 = (base->GetBasicMode() == Config::N80V2);
	
	if (isv2)
		dmac->ConnectRd(mem1->GetTVRAM(), 0xf000, 0x1000);
	else
		dmac->ConnectRd(mem1->GetRAM(), 0, 0x10000);
	dmac->ConnectWr(mem1->GetRAM(), 0, 0x10000);
	
	opn1->SetOPNMode((cfgflags & Config::enableopna) != 0);
	opn1->Enable(isv2 || !(cfgflag2 & Config::disableopn44));
	opn2->SetOPNMode((cfgflags & Config::opnaona8) != 0);
	opn2->Enable((cfgflags & (Config::opnaona8 | Config::opnona8)) != 0);

	if (!isn80v2)
		opn1->SetIMask(0x32, 0x80);
	else
		opn1->SetIMask(0x33, 0x02);

	
	bus1.Out(pres, base->GetBasicMode());
	bus1.Out(0x30, 1);
	bus1.Out(0x30, 0);
	bus1.Out(0x31, 0);
	bus1.Out(0x32, 0x80);
	bus1.Out(0x33, isn80v2 ? 0x82 : 0x02);
	bus1.Out(0x34, 0);
	bus1.Out(0x35, 0);
	bus1.Out(0x40, 0);
	bus1.Out(0x53, 0);
	bus1.Out(0x5f, 0);
	bus1.Out(0x70, 0);
	bus1.Out(0x99, cd ? 0x10 : 0x00);
	bus1.Out(0xe2, 0);
	bus1.Out(0xe3, 0);
	bus1.Out(0xe6, 0);
	bus1.Out(0xf1, 1);
	bus2.Out(pres2, 0);

//	statusdisplay.Show(10, 1000, "CPUMode = %d", cpumode);
}

// ---------------------------------------------------------------------------
//	デバイス接続
//
bool PC88::ConnectDevices()
{
	static const IOBus::Connector c_cpu1[] =
	{
		{ pres, IOBus::portout, Z80::reset },
		{ pirq, IOBus::portout, Z80::irq },
		{ 0, 0, 0 }
	};
	if (!bus1.Connect(&cpu1, c_cpu1)) return false;
	if (!cpu1.Init(&mm1, &bus1, piack)) return false;

	static const IOBus::Connector c_base[] = 
	{
		{ pres, IOBus::portout, Base::reset },
		{ vrtc, IOBus::portout, Base::vrtc },
		{ 0x30, IOBus::portin,  Base::in30 },
		{ 0x31, IOBus::portin,  Base::in31 },
		{ 0x40, IOBus::portin,  Base::in40 },
		{ 0x6e, IOBus::portin,  Base::in6e },
		{ 0, 0, 0 }
	};
	base = new PC8801::Base(DEV_ID('B', 'A', 'S', 'E'));
	if (!base || !bus1.Connect(base, c_base)) return false;
	if (!base->Init(this)) return false;
	devlist.Add(tapemgr);

	static const IOBus::Connector c_dmac[] =
	{
		{ pres, IOBus::portout, PD8257::reset },
		{ 0x60, IOBus::portout, PD8257::setaddr  },
		{ 0x61, IOBus::portout, PD8257::setcount },
		{ 0x62, IOBus::portout, PD8257::setaddr  },
		{ 0x63, IOBus::portout, PD8257::setcount },
		{ 0x64, IOBus::portout, PD8257::setaddr  },
		{ 0x65, IOBus::portout, PD8257::setcount },
		{ 0x66, IOBus::portout, PD8257::setaddr  },
		{ 0x67, IOBus::portout, PD8257::setcount },
		{ 0x68, IOBus::portout, PD8257::setmode  },
		{ 0x60, IOBus::portin,  PD8257::getaddr  },
		{ 0x61, IOBus::portin,  PD8257::getcount },
		{ 0x62, IOBus::portin,  PD8257::getaddr  },
		{ 0x63, IOBus::portin,  PD8257::getcount },
		{ 0x64, IOBus::portin,  PD8257::getaddr  },
		{ 0x65, IOBus::portin,  PD8257::getcount },
		{ 0x66, IOBus::portin,  PD8257::getaddr  },
		{ 0x67, IOBus::portin,  PD8257::getcount },
		{ 0x68, IOBus::portin,  PD8257::getstat  },
		{ 0, 0, 0 }
	};
	dmac = new PC8801::PD8257(DEV_ID('D', 'M', 'A', 'C'));
	if (!bus1.Connect(dmac, c_dmac)) return false;

	static const IOBus::Connector c_crtc[] =
	{
		{ pres, IOBus::portout, CRTC::reset },
		{ 0x50, IOBus::portout, CRTC::out },
		{ 0x51, IOBus::portout, CRTC::out },
		{ 0x50, IOBus::portin,  CRTC::getstatus },
		{ 0x51, IOBus::portin,  CRTC::in },
		{ 0x00, IOBus::portout, CRTC::pcgout },
		{ 0x01, IOBus::portout, CRTC::pcgout },
		{ 0x02, IOBus::portout, CRTC::pcgout },
		{ 0x33,	IOBus::portout, CRTC::setkanamode },
		{ 0, 0, 0 }
	};
	crtc = new PC8801::CRTC(DEV_ID('C', 'R', 'T', 'C'));
	if (!crtc || !bus1.Connect(crtc, c_crtc)) return false;

	static const IOBus::Connector c_mem1[] =
	{
		{ pres, IOBus::portout, Memory::reset },
		{ 0x31, IOBus::portout, Memory::out31 },
		{ 0x32, IOBus::portout, Memory::out32 },
		{ 0x33, IOBus::portout, Memory::out33 },
		{ 0x34, IOBus::portout, Memory::out34 },
		{ 0x35, IOBus::portout, Memory::out35 },
		{ 0x5c, IOBus::portout, Memory::out5x },
		{ 0x5d, IOBus::portout, Memory::out5x },
		{ 0x5e, IOBus::portout, Memory::out5x },
		{ 0x5f, IOBus::portout, Memory::out5x },
		{ 0x70, IOBus::portout, Memory::out70 },
		{ 0x71, IOBus::portout, Memory::out71 },
		{ 0x78, IOBus::portout, Memory::out78 },
		{ 0x99, IOBus::portout, Memory::out99 },
		{ 0xe2, IOBus::portout, Memory::oute2 },
		{ 0xe3, IOBus::portout, Memory::oute3 },
		{ 0xf0, IOBus::portout, Memory::outf0 },
		{ 0xf1, IOBus::portout, Memory::outf1 },
		{ vrtc, IOBus::portout, Memory::vrtc  },
		{ 0x32, IOBus::portin,  Memory::in32  },
		{ 0x33, IOBus::portin,	Memory::in33  },
		{ 0x5c, IOBus::portin,  Memory::in5c  },
		{ 0x70, IOBus::portin,  Memory::in70  },
		{ 0x71, IOBus::portin,  Memory::in71  },
		{ 0xe2, IOBus::portin,  Memory::ine2  },
		{ 0xe3, IOBus::portin,  Memory::ine3  },
		{ 0, 0, 0 }
	};
	mem1 = new PC8801::Memory(DEV_ID('M', 'E', 'M', '1'));
	if (!mem1 || !bus1.Connect(mem1, c_mem1)) return false;
	if (!mem1->Init(&mm1, &bus1, crtc, cpu1.GetWaits())) return false;
	
	if (!crtc->Init(&bus1, this, dmac, draw)) return false;

	static const IOBus::Connector c_knj1[] =
	{
		{ 0xe8, IOBus::portout, KanjiROM::setl  },
		{ 0xe9, IOBus::portout, KanjiROM::seth  },
		{ 0xe8, IOBus::portin,  KanjiROM::readl },
		{ 0xe9, IOBus::portin,  KanjiROM::readh },
		{ 0, 0, 0 }
	};
	knj1 = new PC8801::KanjiROM(DEV_ID('K', 'N', 'J', '1'));
	if (!knj1 || !bus1.Connect(knj1, c_knj1)) return false;
	if (!knj1->Init("kanji1.rom")) return false;
	
	static const IOBus::Connector c_knj2[] =
	{
		{ 0xec, IOBus::portout, KanjiROM::setl  },
		{ 0xed, IOBus::portout, KanjiROM::seth  },
		{ 0xec, IOBus::portin,  KanjiROM::readl },
		{ 0xed, IOBus::portin,  KanjiROM::readh },
		{ 0, 0, 0 }
	};
	knj2 = new PC8801::KanjiROM(DEV_ID('K', 'N', 'J', '2'));
	if (!knj2 || !bus1.Connect(knj2, c_knj2)) return false;
	if (!knj2->Init("kanji2.rom")) return false;

	static const IOBus::Connector c_scrn[] =
	{
		{ pres, IOBus::portout, Screen::reset },
		{ 0x30, IOBus::portout, Screen::out30 },
		{ 0x31, IOBus::portout, Screen::out31 },
		{ 0x32, IOBus::portout, Screen::out32 },
		{ 0x33, IOBus::portout, Screen::out33 },
		{ 0x52, IOBus::portout, Screen::out52 },
		{ 0x53, IOBus::portout, Screen::out53 },
		{ 0x54, IOBus::portout, Screen::out54 },
		{ 0x55, IOBus::portout, Screen::out55to5b },
		{ 0x56, IOBus::portout, Screen::out55to5b },
		{ 0x57, IOBus::portout, Screen::out55to5b },
		{ 0x58, IOBus::portout, Screen::out55to5b },
		{ 0x59, IOBus::portout, Screen::out55to5b },
		{ 0x5a, IOBus::portout, Screen::out55to5b },
		{ 0x5b, IOBus::portout, Screen::out55to5b },
		{ 0, 0, 0 }
	};
	scrn = new PC8801::Screen(DEV_ID('S', 'C', 'R', 'N'));
	if (!scrn || !bus1.Connect(scrn, c_scrn)) return false;
	if (!scrn->Init(&bus1, mem1, crtc)) return false;

	static const IOBus::Connector c_intc[] =
	{
		{ pres,  IOBus::portout, INTC::reset },
		{ pint0, IOBus::portout, INTC::request },
		{ pint1, IOBus::portout, INTC::request },
		{ pint2, IOBus::portout, INTC::request },
		{ pint3, IOBus::portout, INTC::request },
		{ pint4, IOBus::portout, INTC::request },
		{ pint5, IOBus::portout, INTC::request },
		{ pint6, IOBus::portout, INTC::request },
		{ pint7, IOBus::portout, INTC::request },
		{ 0xe4,  IOBus::portout, INTC::setreg },
		{ 0xe6,  IOBus::portout, INTC::setmask },
		{ piack, IOBus::portin,  INTC::intack },
		{ 0, 0, 0 }
	};
	intc = new PC8801::INTC(DEV_ID('I', 'N', 'T', 'C'));
	if (!intc || !bus1.Connect(intc, c_intc)) return false;
	if (!intc->Init(&bus1, pirq, pint0)) return false;

	static const IOBus::Connector c_subsys[] =
	{
		{ pres,  IOBus::portout, SubSystem::reset },
		{ 0xfc,  IOBus::portout | IOBus::sync, SubSystem::m_set0 },
		{ 0xfd,  IOBus::portout | IOBus::sync, SubSystem::m_set1 },
		{ 0xfe,  IOBus::portout | IOBus::sync, SubSystem::m_set2 },
		{ 0xff,  IOBus::portout | IOBus::sync, SubSystem::m_setcw },
		{ 0xfc,  IOBus::portin  | IOBus::sync, SubSystem::m_read0 },
		{ 0xfd,  IOBus::portin  | IOBus::sync, SubSystem::m_read1 },
		{ 0xfe,  IOBus::portin  | IOBus::sync, SubSystem::m_read2 },
		{ 0, 0, 0 }
	};
	subsys = new PC8801::SubSystem(DEV_ID('S', 'U', 'B', ' '));
	if (!subsys || !bus1.Connect(subsys, c_subsys)) return false;

	static const IOBus::Connector c_sio[] =
	{
		{ pres,  IOBus::portout, SIO::reset },
		{ 0x20,  IOBus::portout, SIO::setdata },
		{ 0x21,  IOBus::portout, SIO::setcontrol },
		{ psioin,IOBus::portout, SIO::acceptdata },
		{ 0x20,  IOBus::portin,  SIO::getdata },
		{ 0x21,  IOBus::portin,  SIO::getstatus },
		{ 0, 0, 0 }
	};
	siotape = new PC8801::SIO(DEV_ID('S', 'I', 'O', ' '));
	if (!siotape || !bus1.Connect(siotape, c_sio)) return false;
	if (!siotape->Init(&bus1, pint0, psioreq)) return false;

	static const IOBus::Connector c_tape[] =
	{
		{ psioreq,	IOBus::portout, TapeManager::requestdata },
		{ 0x30,		IOBus::portout,	TapeManager::out30 },
		{ 0x40,		IOBus::portin,	TapeManager::in40 },
		{ 0, 0, 0 }
	};
	if (!bus1.Connect(tapemgr, c_tape)) return false;
	if (!tapemgr->Init(this, &bus1, psioin)) return false;

	static const IOBus::Connector c_opn1[] =
	{
		{ pres, IOBus::portout, OPNIF::reset },
		{ 0x32, IOBus::portout, OPNIF::setintrmask },
		{ 0x44, IOBus::portout, OPNIF::setindex0 },
		{ 0x45, IOBus::portout, OPNIF::writedata0 },
		{ 0x46, IOBus::portout, OPNIF::setindex1 },
		{ 0x47, IOBus::portout, OPNIF::writedata1 },
		{ ptimesync, IOBus::portout, OPNIF::sync },
		{ 0x44, IOBus::portin,  OPNIF::readstatus },
		{ 0x45, IOBus::portin,  OPNIF::readdata0 },
		{ 0x46, IOBus::portin,  OPNIF::readstatusex },
		{ 0x47, IOBus::portin,  OPNIF::readdata1 },
		{ 0, 0, 0 }
	};
	opn1 = new PC8801::OPNIF(DEV_ID('O', 'P', 'N', '1'));
	if (!opn1 || !opn1->Init(&bus1, pint4, popnio, this)) return false;
	if (!bus1.Connect(opn1, c_opn1)) return false;
	opn1->SetIMask(0x32, 0x80);

	static const IOBus::Connector c_opn2[] =
	{
		{ pres, IOBus::portout, OPNIF::reset },
		{ 0xaa, IOBus::portout, OPNIF::setintrmask },
		{ 0xa8, IOBus::portout, OPNIF::setindex0 },
		{ 0xa9, IOBus::portout, OPNIF::writedata0 },
		{ 0xac, IOBus::portout, OPNIF::setindex1 },
		{ 0xad, IOBus::portout, OPNIF::writedata1 },
		{ 0xa8, IOBus::portin,  OPNIF::readstatus },
		{ 0xa9, IOBus::portin,  OPNIF::readdata0 },
		{ 0xac, IOBus::portin,  OPNIF::readstatusex },
		{ 0xad, IOBus::portin,  OPNIF::readdata1 },
		{ 0, 0, 0 }
	};
	opn2 = new PC8801::OPNIF(DEV_ID('O', 'P', 'N', '2'));
	if (!opn2->Init(&bus1, pint4, popnio, this)) return false;
	if (!opn2 || !bus1.Connect(opn2, c_opn2)) return false;
	opn2->SetIMask(0xaa, 0x80);
	
	static const IOBus::Connector c_caln[] = 
	{
		{ pres, IOBus::portout, Calender::reset },
		{ 0x10, IOBus::portout, Calender::out10 },
		{ 0x40, IOBus::portout, Calender::out40 },
		{ 0x40, IOBus::portin,  Calender::in40 },
		{ 0, 0, 0 }
	};
	caln = new PC8801::Calender(DEV_ID('C', 'A', 'L', 'N'));
	if (!caln || !caln->Init()) return false;
	if (!bus1.Connect(caln, c_caln)) return false;

	static const IOBus::Connector c_beep[] = 
	{
		{ 0x40, IOBus::portout, Beep::out40 },
		{ 0, 0, 0 }
	};
	beep = new PC8801::Beep(DEV_ID('B', 'E', 'E', 'P'));
	if (!beep || !beep->Init()) return false;
	if (!bus1.Connect(beep, c_beep)) return false;


	static const IOBus::Connector c_siom[] =
	{
		{ pres,  IOBus::portout, SIO::reset },
		{ 0xc2,  IOBus::portout, SIO::setdata },
		{ 0xc3,  IOBus::portout, SIO::setcontrol },
		{ 0,	 IOBus::portout, SIO::acceptdata },
		{ 0xc2,  IOBus::portin,  SIO::getdata },
		{ 0xc3,  IOBus::portin,  SIO::getstatus },
		{ 0, 0, 0 }
	};
	siomidi = new PC8801::SIO(DEV_ID('S', 'I', 'O', 'M'));
	if (!siomidi || !bus1.Connect(siomidi, c_siom)) return false;
	if (!siomidi->Init(&bus1, 0, psioreq)) return false;

	static const IOBus::Connector c_joy[] = 
	{
		{ popnio,	IOBus::portin, JoyPad::getdir },
		{ popnio2,	IOBus::portin, JoyPad::getbutton },
		{ vrtc,		IOBus::portout, JoyPad::vsync },
		{ 0, 0, 0 }
	};
	joypad = new PC8801::JoyPad();//DEV_ID('J', 'O', 'Y', ' '));
	if (!joypad) return false;
	if (!bus1.Connect(joypad, c_joy)) return false;

	return true;
}

// ---------------------------------------------------------------------------
//	デバイス接続(サブCPU)
//
bool PC88::ConnectDevices2()
{
	static const IOBus::Connector c_cpu2[] =
	{
		{ pres2, IOBus::portout, Z80::reset },
		{ pirq2, IOBus::portout, Z80::irq },
		{ 0, 0, 0 }
	};
	if (!bus2.Connect(&cpu2, c_cpu2)) return false;
	if (!cpu2.Init(&mm2, &bus2, piac2)) return false;

	static const IOBus::Connector c_mem2[] =
	{
		{ piac2, IOBus::portin, SubSystem::intack },
		{ 0xfc,  IOBus::portout | IOBus::sync, SubSystem::s_set0 },
		{ 0xfd,  IOBus::portout | IOBus::sync, SubSystem::s_set1 },
		{ 0xfe,  IOBus::portout | IOBus::sync, SubSystem::s_set2 },
		{ 0xff,  IOBus::portout | IOBus::sync, SubSystem::s_setcw },
		{ 0xfc,  IOBus::portin  | IOBus::sync, SubSystem::s_read0 },
		{ 0xfd,  IOBus::portin  | IOBus::sync, SubSystem::s_read1 },
		{ 0xfe,  IOBus::portin  | IOBus::sync, SubSystem::s_read2 },
		{ 0, 0, 0 }
	};
	if (!subsys || !bus2.Connect(subsys, c_mem2)) return false;
	if (!subsys->Init(&mm2)) return false;

	static const IOBus::Connector c_fdc[] =
	{
		{ pres2, IOBus::portout, FDC::reset },
		{ 0xfb,  IOBus::portout, FDC::setdata },
		{ 0xf4,  IOBus::portout, FDC::drivecontrol },
		{ 0xf8,  IOBus::portout, FDC::motorcontrol },
		{ 0xf8,  IOBus::portin,  FDC::tcin },
		{ 0xfa,  IOBus::portin,  FDC::getstatus },
		{ 0xfb,  IOBus::portin,  FDC::getdata },
		{ 0, 0, 0 }
	};
	fdc = new PC8801::FDC(DEV_ID('F', 'D', 'C', ' '));
	if (!bus2.Connect(fdc, c_fdc)) return false;
	if (!fdc->Init(diskmgr, this, &bus2, pirq2, pfdstat)) return false;
	
	return true;
}

// ---------------------------------------------------------------------------
//	設定反映
//
void PC88::ApplyConfig(Config* cfg)
{
	cfgflags = cfg->flags;
	cfgflag2 = cfg->flag2;
	
	base->SetSwitch(cfg);
	scrn->ApplyConfig(cfg);
	mem1->ApplyConfig(cfg);
	crtc->ApplyConfig(cfg);
	fdc->ApplyConfig(cfg);
	beep->EnableSING(!(cfg->flags & Config::disablesing));
	opn1->SetFMMixMode(!!(cfg->flag2 & Config::usefmclock));
	opn1->SetVolume(cfg);
	opn2->SetFMMixMode(!!(cfg->flag2 & Config::usefmclock));
	opn2->SetVolume(cfg);
	
	cpumode = (cfg->cpumode == Config::msauto)
		? (cfg->mainsubratio > 1 ? ms21 : ms11)
		: (cfg->cpumode & 1);
	if ((cfg->flags & Config::subcpucontrol) != 0)
		cpumode |= stopwhenidle;

	if (cfg->flags & PC8801::Config::enablepad)
	{
		joypad->SetButtonMode(cfg->flags & Config::swappadbuttons ? JoyPad::SWAPPED : JoyPad::NORMAL);
	}
	else
	{
		joypad->SetButtonMode(JoyPad::DISABLED);
	}

//	EnablePad((cfg->flags & PC8801::Config::enablepad) != 0);
//	if (padenable)
//		cfg->flags &= ~PC8801::Config::enablemouse;
//	EnableMouse((cfg->flags & PC8801::Config::enablemouse) != 0);

}

// ---------------------------------------------------------------------------
//	音量変更
//
void PC88::SetVolume(PC8801::Config* cfg)
{
	opn1->SetVolume(cfg);
	opn2->SetVolume(cfg);
}

// ---------------------------------------------------------------------------
//	1 フレーム分の時間を求める．
//
int PC88::GetFramePeriod()
{
	return crtc ? crtc->GetFramePeriod() : 100000 / 60;
}

// ---------------------------------------------------------------------------
//	仮想時間と現実時間の同期を取ったときに呼ばれる
//
void PC88::TimeSync()
{
	bus1.Out(ptimesync, 0);
}

bool PC88::IsN80Supported()
{
	return mem1->IsN80Ready();
}

bool PC88::IsN80V2Supported()
{
	return mem1->IsN80V2Ready();
}
