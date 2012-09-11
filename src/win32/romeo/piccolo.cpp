//	$Id: piccolo.cpp,v 1.3 2003/04/22 13:16:36 cisc Exp $

#include "headers.h"
#include <winioctl.h>
#include "piccolo.h"
#include "romeo.h"
#include "misc.h"
#include "status.h"
#include "../../piccolo/piioctl.h"

#define LOGNAME "piccolo"
#include "diag.h"

struct PCIDRV
{
	HMODULE			mod;
	PCIFINDDEV		finddev;
	PCICFGREAD32	read32;
	PCIMEMWR8		out8;
	PCIMEMWR16		out16;
	PCIMEMWR32		out32;
	PCIMEMRD8		in8;
	PCIMEMRD16		in16;
	PCIMEMRD32		in32;
};

#define	ROMEO_TPTR(member)	(int)&(((PCIDRV *)NULL)->member)

struct DLLPROCESS
{
	char	*symbol;
	int		addr;
};

static const DLLPROCESS	dllproc[] = 
{
	{FN_PCIFINDDEV,		ROMEO_TPTR(finddev)},
	{FN_PCICFGREAD32,	ROMEO_TPTR(read32)},
	{FN_PCIMEMWR8,		ROMEO_TPTR(out8)},
	{FN_PCIMEMWR16,		ROMEO_TPTR(out16)},
	{FN_PCIMEMWR32,		ROMEO_TPTR(out32)},
	{FN_PCIMEMRD8,		ROMEO_TPTR(in8)},
	{FN_PCIMEMRD16,		ROMEO_TPTR(in16)},
	{FN_PCIMEMRD32,		ROMEO_TPTR(in32)}
};

PCIDRV pcidrv = { 0, };

static bool LoadDLL()
{
	if (!pcidrv.mod)
	{
		pcidrv.mod = LoadLibrary(PCIDEBUG_DLL);
		if (!pcidrv.mod)
			return false;
		
		FARPROC proc;
		for (int i=0; i<sizeof(dllproc)/sizeof(DLLPROCESS); i++)
		{
			proc = GetProcAddress(pcidrv.mod, dllproc[i].symbol);
			*(DWORD *)(((BYTE *)&pcidrv) + dllproc[i].addr) = (DWORD) proc;
			if (!proc) 
				break;
		}
		if (!proc)
		{
			FreeLibrary(pcidrv.mod);
			pcidrv.mod = 0;
		}
	}
	return !!pcidrv.mod;
}


// ---------------------------------------------------------------------------

Piccolo Piccolo::piccolo;


// ---------------------------------------------------------------------------
//
//
Piccolo::Piccolo()
: active(false), hthread(0), idthread(0), hfile(INVALID_HANDLE_VALUE)
{
	avail = Init();
}

Piccolo::~Piccolo()
{
	Cleanup();
}

Piccolo* Piccolo::GetInstance()
{
	if (piccolo.avail >= 0) 
		return &piccolo;
	return 0;
}

// ---------------------------------------------------------------------------
//
//
int Piccolo::Init()
{
	// piccolo.sys はいますか？
	hfile = CreateFile(
		"\\\\.\\Romeo",           // Open the Device "file"
		GENERIC_WRITE | GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	islegacy = false;

	if (hfile == INVALID_HANDLE_VALUE)
	{
		// 従来の方法で。
		// DLL 用意
		Log("LoadDLL\n");
		if (!LoadDLL())
			return PICCOLOE_DLL_NOT_FOUND;

		islegacy = true;

		// ROMEO の存在確認
		// デバイスを探す
		Log("FindDevice\n");
		uint32 id;
		id = pcidrv.finddev(0x6809, 0x8121, 0);
		if (id & 0xff)
			id = pcidrv.finddev(0x6809, 0x2151, 0);
		Log(" ID = %.8x\n", id);
		if (id & 0xff)
			return PICCOLOE_ROMEO_NOT_FOUND;

		// ROMEO はありそうだが、デバイスは？
		id >>= 16;
		addr = pcidrv.read32(id, ROMEO_BASEADDRESS1);
		irq  = pcidrv.read32(id, ROMEO_PCIINTERRUPT) & 0xff;
		Log(" ADDR = %.8x\n", addr);
		Log(" IRQ  = %.8x\n", irq);
		
		if (!addr)
			return PICCOLOE_ROMEO_NOT_FOUND;

	}
	ymf288.Init(this, addr + ROMEO_YMF288BASE);
	ymf288.Reset();

	// thread 作成
	shouldterminate = false;
	if (!hthread)
	{
		hthread = (HANDLE) 
			_beginthreadex(NULL, 0, ThreadEntry, 
				reinterpret_cast<void*>(this), 0, &idthread);
	}
	if (!hthread)
		return PICCOLOE_THREAD_ERROR;

	SetMaximumLatency(1000000);
	SetLatencyBufferSize(16384);
	return PICCOLO_SUCCESS;
}

// ---------------------------------------------------------------------------
//	後始末
//
void Piccolo::Cleanup()
{
	if (hthread)
	{
		shouldterminate = true;
		if (WAIT_TIMEOUT == WaitForSingleObject(hthread, 3000))
		{
			TerminateThread(hthread, 0);
		}
		CloseHandle(hthread);
		hthread = 0;
	}
	{
		if (hfile)
			CloseHandle(hfile);
	}
}

// ---------------------------------------------------------------------------
//	Core Thread
//
uint Piccolo::ThreadMain()
{
	::SetThreadPriority(hthread, THREAD_PRIORITY_TIME_CRITICAL);
	while (!shouldterminate)
	{
		Event* ev;
		const int waitdefault = 2;
		int wait = waitdefault;
		{
			CriticalSection::Lock lock(cs);
			uint32 time = GetCurrentTime();
			while ((ev = Top()) && !shouldterminate)
			{
				int32 d = ev->at - time;

				if (d >= 1000)
				{
					if (d > maxlatency)
						d = maxlatency;
					wait = d / 1000;
					break;
				}
				ev->drv->Set(ev->addr, ev->data);
				Pop();
			}
		}
		if (wait > waitdefault)
			wait = waitdefault;
		Sleep(wait);
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	キューに追加
//
bool Piccolo::Push(Piccolo::Event& ev)
{
	if ((evwrite + 1) % eventries == evread)
		return false;
	events[evwrite] = ev;
	evwrite = (evwrite + 1) % eventries;
	return true;
}

// ---------------------------------------------------------------------------
//	キューから一個貰う
//
Piccolo::Event* Piccolo::Top()
{
	if (evwrite == evread)
		return 0;
	
	return &events[evread];
}

void Piccolo::Pop()
{
	evread = (evread + 1) % eventries;
}


// ---------------------------------------------------------------------------
//	サブスレッド開始点
//
uint CALLBACK Piccolo::ThreadEntry(void* arg)
{
	return reinterpret_cast<Piccolo*>(arg)->ThreadMain();
}

// ---------------------------------------------------------------------------
//
//
bool Piccolo::SetLatencyBufferSize(uint entries)
{
	CriticalSection::Lock lock(cs);
	Event* ne = new Event[entries];
	if (!ne)
		return false;

	delete[] events;
	events = ne;
	eventries = entries;
	evread = 0;
	evwrite = 0;
	return true;
}

bool Piccolo::SetMaximumLatency(uint nanosec)
{
	maxlatency = nanosec;
	return true;
}

uint32 Piccolo::GetCurrentTime()
{
	return ::GetTickCount() * 1000;
}

int Piccolo::GetChip(PICCOLO_CHIPTYPE type, PiccoloChip** pc)
{
	*pc = 0;
	Log("GetChip %d\n", type);
	if (type != PICCOLO_YMF288)
		return PICCOLOE_HARDWARE_NOT_AVAILABLE;
	
	Log(" Type: YMF288\n");
	if (!ymf288.IsAvailable())
		return PICCOLOE_HARDWARE_IN_USE;
	Log(" allocated\n");
	ymf288.Reserve(true);
	*pc = new ChipIF(this, &ymf288);
	return PICCOLO_SUCCESS;
}

// ---------------------------------------------------------------------------

int Piccolo::DrvInit(Driver* drv, uint param)
{
	drv->Reset();
	return true;
}

void Piccolo::DrvReset(Driver* drv)
{
	CriticalSection::Lock lock(cs);
	drv->Reset();
	// 本当は該当するエントリだけ削除すべきだが…
	evread = 0;
	evwrite = 0;
}

bool Piccolo::DrvSetReg(Driver* drv, uint32 at, uint addr, uint data)
{
	if (int32(at - GetCurrentTime()) > maxlatency)
	{
//		statusdisplay.Show(100, 0, "Piccolo: Time %.6d", at - GetCurrentTime());
		return false;
	}
	Event ev;
	ev.drv = drv;
	ev.at = at;
	ev.addr = addr;
	ev.data = data;

/*int d = evwrite - evread;
if (d < 0) d += eventries;
statusdisplay.Show(100, 0, "Piccolo: Time %.6d  Buf: %.6d  R:%.8d W:%.8d w:%.6d", at - GetCurrentTime(), d, evread, GetCurrentTime(), asleep);
*/
	return Push(ev);
}

void Piccolo::DrvRelease(Driver* drv)
{
	drv->Reserve(false);
}

// ---------------------------------------------------------------------------


void Piccolo::YMF288::Reset() 
{
	Log("YMF288::Reset()\n");
	Mute();
	if (piccolo->islegacy)
	{
		pcidrv.out32(addr + CTRL, 0x00);
		Sleep(150);
		pcidrv.out32(addr + CTRL, 0x80);
		Sleep(150);
	}
}

bool Piccolo::YMF288::IsBusy() 
{
	if (piccolo->islegacy)
		return (pcidrv.in8(addr + ADDR0) & 0x80) != 0;
	return false;
}

void Piccolo::YMF288::Mute()
{
	Log("YMF288::Mute()\n");
	Set(0x007, 0x3f);
	for (uint r=0x40; r<0x4f; r++)
	{
		if (~r & 3)
		{
			Set(0x000 + r, 0x7f);
			Set(0x100 + r, 0x7f);
		}
	}
}

void Piccolo::YMF288::Set(uint a, uint d)
{
	if (piccolo->islegacy)
	{
		while (IsBusy())
			Sleep(0);
		pcidrv.out8(addr + (a < 0x100 ? ADDR0 : ADDR1), a & 0xff);

		while (IsBusy())
			Sleep(0);
		pcidrv.out8(addr + (a < 0x100 ? DATA0 : DATA1), d & 0xff);
	}
	else if (piccolo->hfile != INVALID_HANDLE_VALUE)
	{
		ROMEO_WRITE_INPUT	rwi;
		LONG				ioctlcode;
		ULONG				datalength;
		ULONG				returnedlength;

		ioctlcode = IOCTL_ROMEO_OPN3_SETREG;
		rwi.PortNumber = a;
		rwi.CharData = d;
		datalength = sizeof(rwi);

		BOOL result;
		result = DeviceIoControl(
						piccolo->hfile,					// Handle to device
						ioctlcode,				// IO Control code for Write
						&rwi,					// Buffer to driver.  Holds port & data.
						datalength,				// Length of buffer in bytes.
						NULL,					// Buffer from driver.   Not used.
						0,						// Length of buffer in bytes.
						&returnedlength,		// Bytes placed in outbuf.  Should be 0.
						NULL					// NULL means wait till I/O completes.
						);

	}
}
