//	$Id: winexapi.cpp,v 1.1 2002/04/07 05:40:11 cisc Exp $

#include "headers.h"

#define DECLARE_EXAPI(name, type, arg, dll, api, def) \
	static type WINAPI ef_##name arg { return def; } \
	ExtendedAPIAccess <type (WINAPI*) arg, ExtendedAPIAccessBase> name(dll, api, ef_##name);

#define DECLARE_EXDLL(name, type, arg, dll, api, def) \
	static type WINAPI ef_##name arg { return def; } \
	ExtendedAPIAccess <type (WINAPI*) arg, ExternalDLLAccessBase> name(dll, api, ef_##name);

#include "winexapi.h"

ExtendedAPIAccessBase::ExtendedAPIAccessBase(const char* dllname, const char* apiname, void* dummy)
{
	method = dummy;
	valid = false;
	HMODULE hmod = ::GetModuleHandle(dllname);
	if (hmod)
	{
		method = ::GetProcAddress(hmod, apiname);
		if (!method)
			method = dummy;
		valid = true;
	}
}

void ExternalDLLAccessBase::Load()
{
	method = dummy;
	hmod = ::LoadLibrary(dllname);
	if (hmod)
	{
		void* addr = GetProcAddress(hmod, apiname);
		if (addr)
			method = addr;
		else
			::FreeLibrary(hmod), hmod = 0;
	}
}

ExternalDLLAccessBase::~ExternalDLLAccessBase() 
{
	if (hmod)
		::FreeLibrary(hmod);
}
