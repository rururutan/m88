// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: module.cpp,v 1.1 1999/10/10 01:47:21 cisc Exp $

#include "headers.h"
#include "module.h"

using namespace PC8801;

ExtendModule::ExtendModule()
: hdll(0), mod(0)
{
}

ExtendModule::~ExtendModule()
{
	Disconnect();
}

bool ExtendModule::Connect(const char* dllname, ISystem* pc)
{
	Disconnect();

	hdll = LoadLibrary(dllname);
	if (!hdll) return false;

	F_CONNECT2 conn = (F_CONNECT2) GetProcAddress(hdll, "M88CreateModule");
	if (!conn) return false;
	
	mod = (*conn)(pc);
	return mod != 0;
}

bool ExtendModule::Disconnect()
{
	if (mod)
	{
		mod->Release(), mod = 0;
	}
 	if (hdll)
	{
		FreeLibrary(hdll), hdll = 0;
	}
	return true;
}

ExtendModule* ExtendModule::Create(const char* dllname, ISystem* pc)
{
	ExtendModule* em = new ExtendModule;
	if (em)
	{
		if (em->Connect(dllname, pc))
			return em;
		delete em;
	}
	return 0;
}
