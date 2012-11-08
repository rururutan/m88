// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: 88config.h,v 1.9 2000/08/06 09:59:20 cisc Exp $

#pragma once

#include "if/ifcommon.h"
#include "pc88/config.h"

namespace PC8801
{
	void SaveConfig(Config* cfg, const char* inifile, bool writedefault);
	void LoadConfig(Config* cfg, const char* inifile, bool applydefault);
	void LoadConfigDirectory(Config* cfg, const char* inifile, const char* entry, bool readalways);
}

