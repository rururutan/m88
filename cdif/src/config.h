// $Id: config.h,v 1.1 1999/12/28 11:25:53 cisc Exp $

#ifndef incl_config_h
#define incl_config_h

#include "if/ifcommon.h"
#include "instthnk.h"

class ConfigCDIF : public IConfigPropSheet
{
public:
	ConfigCDIF();
	bool Init(HINSTANCE _hinst);  
	bool IFCALL Setup(IConfigPropBase*, PROPSHEETPAGE* psp);
	
private:
	BOOL PageProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK PageGate(ConfigCDIF*, HWND, UINT, WPARAM, LPARAM);
	
	HINSTANCE hinst;
	IConfigPropBase* base;
	InstanceThunk gate;
};

#endif // incl_config_h
