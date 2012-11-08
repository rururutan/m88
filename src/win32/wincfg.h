// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: wincfg.h,v 1.4 2003/05/12 22:26:35 cisc Exp $

#pragma once

#include "if/ifcommon.h"
#include "pc88/config.h"
#include "cfgpage.h"

namespace PC8801
{

class WinConfig : public IConfigPropBase
{
public:
	WinConfig();
	~WinConfig();
	
	bool Show(HINSTANCE, HWND, Config* config);
	bool ProcMsg(MSG& msg);
	bool IsOpen() { return !!hwndps; }
	void Close();

private:
	struct PPNode
	{
		PPNode* next;
		IConfigPropSheet* sheet;
	};

	bool IFCALL Add(IConfigPropSheet* sheet);
	bool IFCALL Remove(IConfigPropSheet* sheet);

	bool IFCALL Apply();
	bool IFCALL PageSelected(IConfigPropSheet*);
	bool IFCALL PageChanged(HWND);
	void IFCALL _ChangeVolume(bool);

	int PropProc(HWND, UINT, LPARAM);
	static int CALLBACK PropProcGate(HWND, UINT, LPARAM);
	static WinConfig* instance;

	PPNode* pplist;
	typedef vector<IConfigPropSheet*> PropSheets;
	PropSheets propsheets;
	
	HWND hwndparent;
	HWND hwndps;
	HINSTANCE hinst;
	Config config;
	Config orgconfig;
	int page;
	int npages;

	ConfigCPU ccpu;
	ConfigScreen cscrn;
	ConfigSound csound;
	ConfigVolume cvol;
	ConfigFunction cfunc;
	ConfigSwitch cswitch;
	ConfigEnv cenv;
	ConfigROMEO cromeo;
};

}

