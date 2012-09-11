// $Id: config.h,v 1.2 1999/12/28 10:33:39 cisc Exp $

#ifndef incl_config_h
#define incl_config_h

#include "if/ifcommon.h"
#include "instthnk.h"

class ConfigMP : public IConfigPropSheet
{
public:
	ConfigMP();
	bool Init(HINSTANCE _hinst);  
	bool IFCALL Setup(IConfigPropBase*, PROPSHEETPAGE* psp);
	
private:
	BOOL PageProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK PageGate(ConfigMP*, HWND, UINT, WPARAM, LPARAM);
	
	HINSTANCE hinst;
	IConfigPropBase* base;
	InstanceThunk gate;
};

#endif // incl_config_h
