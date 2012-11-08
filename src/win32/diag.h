// ----------------------------------------------------------------------------
//	diag.h
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	$Id: diag.h,v 1.9 2002/04/07 05:40:10 cisc Exp $
//

#pragma once

class Diag
{
public:
	Diag(const char* logname);
	~Diag();
	void Put(const char*, ...);
	static int GetCPUTick();

private:
	FILE* file;
};


#if defined(_DEBUG) && defined(LOGNAME)
	static Diag diag__(LOGNAME".dmp");
	#define LOG0 diag__.Put
	#define LOG1 diag__.Put
	#define LOG2 diag__.Put
	#define LOG3 diag__.Put
	#define LOG4 diag__.Put
	#define LOG5 diag__.Put
	#define LOG6 diag__.Put
	#define LOG7 diag__.Put
	#define LOG8 diag__.Put
	#define LOG9 diag__.Put
	#define DIAGINIT(z) //Diag::Init(z)
	#define LOGGING
	#define Log diag__.Put
#else
	#define LOG0(m)		void (0)
	#define LOG1(m,a)	void (0)
	#define LOG2(m,a,b)	void (0)
	#define LOG3(m,a,b,c)	void (0)
	#define LOG4(m,a,b,c,d)	void (0)
	#define LOG5(m,a,b,c,d,e)	void (0)
	#define LOG6(m,a,b,c,d,e,f)	void (0)
	#define LOG7(m,a,b,c,d,e,f,g)	void (0)
	#define LOG8(m,a,b,c,d,e,f,g,h)	void (0)
	#define LOG9(m,a,b,c,d,e,f,g,h,i)	void (0)
	#define DIAGINIT(z)
	#define Log 0 ? 0 : 
#endif

