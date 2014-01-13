//	$Id: piccolo.cpp,v 1.3 2003/04/22 13:16:36 cisc Exp $

#include "headers.h"
#include <winioctl.h>
#include "piccolo.h"
#include "piccolo_romeo.h"
#include "piccolo_gimic.h"
#include "romeo.h"
#include "misc.h"
#include "status.h"
#include "../../piccolo/piioctl.h"

#define LOGNAME "piccolo"
#include "diag.h"

Piccolo* Piccolo::instance = NULL;

// ---------------------------------------------------------------------------
//
//
Piccolo* Piccolo::GetInstance()
{
	if ( instance ) {
		return instance;
	} else {
		instance = new Piccolo_Romeo();
		if ( instance->Init() == PICCOLO_SUCCESS ) {
			return instance;
		}
		delete instance;
		instance = new Piccolo_Gimic();
		if ( instance->Init() == PICCOLO_SUCCESS ) {
			return instance;
		}
		delete instance;
		instance = 0;
	}

	return 0;
}

void Piccolo::DeleteInstance()
{
	if (instance) {
		delete instance;
		instance = 0;
	}
}

Piccolo::Piccolo()
: active(false), hthread(0), idthread(0), avail(0), events(0), evread(0), evwrite(0), eventries(0),
  maxlatency(0), shouldterminate(true)
{
}

Piccolo::~Piccolo()
{
}

// ---------------------------------------------------------------------------
//
//
int Piccolo::Init()
{
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
		const int waitdefault = 1;
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
				SetReg(ev->addr, ev->data);
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
	return ::timeGetTime() * 1000;
}

// ---------------------------------------------------------------------------

void Piccolo::DrvReset()
{
	CriticalSection::Lock lock(cs);

	// 本当は該当するエントリだけ削除すべきだが…
	evread = 0;
	evwrite = 0;
}

bool Piccolo::DrvSetReg(uint32 at, uint addr, uint data)
{
	if (int32(at - GetCurrentTime()) > maxlatency)
	{
//		statusdisplay.Show(100, 0, "Piccolo: Time %.6d", at - GetCurrentTime());
		return false;
	}

	Event ev;
	ev.at = at;
	ev.addr = addr;
	ev.data = data;

/*int d = evwrite - evread;
if (d < 0) d += eventries;
statusdisplay.Show(100, 0, "Piccolo: Time %.6d  Buf: %.6d  R:%.8d W:%.8d w:%.6d", at - GetCurrentTime(), d, evread, GetCurrentTime(), asleep);
*/
	return Push(ev);
}

void Piccolo::DrvRelease()
{
}
