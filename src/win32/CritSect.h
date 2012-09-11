// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	CriticalSection Class for Win32
// ---------------------------------------------------------------------------
//	$Id: CritSect.h,v 1.3 1999/08/01 14:19:13 cisc Exp $

#ifndef Win32_CriticalSection_h
#define Win32_CriticalSection_h

class CriticalSection
{
public:
	class Lock
	{
		CriticalSection& cs;
	public:
		Lock(CriticalSection& c) : cs(c) { cs.lock(); }
		~Lock() { cs.unlock(); }
	};

	CriticalSection() { InitializeCriticalSection(&css); }
	~CriticalSection() { DeleteCriticalSection(&css); }

	void lock() { EnterCriticalSection(&css); }
	void unlock() { LeaveCriticalSection(&css); }

private:
	CRITICAL_SECTION css;
};

#endif // Win32_CriticalSection_h
