// ----------------------------------------------------------------------------
//	diag.cpp
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	$Id: diag.cpp,v 1.11 1999/11/26 10:14:08 cisc Exp $
//
#include "headers.h"

#define INCLDIAG
#include "diag.h"

#if defined(_DEBUG)

//Z80C* Diag::cpu = 0;

Diag::Diag(const char* logname)
{
	file = fopen(logname, "w");
}

Diag::~Diag()
{
	if (file)
		fclose(file);
}

void Diag::Put(const char* arg, ...)
{
	va_list va;
	va_start(va, arg);

	if (file)
		vfprintf(file, arg, va);
	va_end(va);
}

int Diag::GetCPUTick()
{
	return 0;
//	return cpu ? cpu->GetCount() : 0;
}

#endif
