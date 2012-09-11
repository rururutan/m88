// ---------------------------------------------------------------------------
//	CriticalSection Class for OS/2
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: critsectos2.h,v 1.1 2001/02/21 11:58:54 cisc Exp $

#ifndef os2_critsect_h
#define os2_critsect_h

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

	CriticalSection() { ::DosCreateMutexSem(0, &handle, 0, false); }
	~CriticalSection() { ::DosCloseMutexSem(handle); }
	void lock()  { ::DosRequestMutexSem(handle, SEM_INDEFINITE_WAIT); }
	void unlock() { ::DosReleaseMutexSem(handle); }
	
private:
	HMTX handle;
};

#endif // os2_critsect_h
