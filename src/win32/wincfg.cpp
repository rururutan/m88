// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: wincfg.cpp,v 1.8 2003/05/12 22:26:35 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "wincfg.h"
#include "misc.h"
#include "messages.h"

using namespace PC8801;

WinConfig* WinConfig::instance = 0;

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinConfig::WinConfig()
: pplist(0), npages(0),
  ccpu(config, orgconfig),
  cscrn(config, orgconfig),
  csound(config, orgconfig),
  cvol(config, orgconfig),
  cfunc(config, orgconfig),
  cswitch(config, orgconfig),
  cenv(config, orgconfig),
  cromeo(config, orgconfig)
{
	page = 0;
	hwndps = 0;

	Add(&ccpu);
	Add(&cscrn);
	Add(&csound);
	Add(&cvol);
	Add(&cswitch);
	Add(&cfunc);
	Add(&cenv);
	Add(&cromeo);
	instance = this;
}

WinConfig::~WinConfig()
{
	propsheets.clear();
	instance = 0;
}

// ---------------------------------------------------------------------------
//	設定ダイアログの実行
//
bool WinConfig::Show(HINSTANCE hinstance, HWND hwnd, Config* conf)
{
	if (!hwndps)
	{
		orgconfig = config = *conf;
		hinst = hinstance;
		hwndparent = hwnd;

		PROPSHEETHEADER psh;
		PROPSHEETPAGE* psp = new PROPSHEETPAGE[propsheets.size()];
		if (!psp)
			return false;

		ccpu.Init(hinst);
		cscrn.Init(hinst);
		csound.Init(hinst);
		cvol.Init(hinst);
		cfunc.Init(hinst);
		cswitch.Init(hinst);
		cenv.Init(hinst);
		cromeo.Init(hinst);

		// 拡張モジュールの場合、headerのバージョンによって PROPSHEETPAGE のサイズが違ったりする
		PROPSHEETPAGE tmppage[2];	// 2個分確保するのは、拡張側の PROPSHEETPAGE のサイズが 
									// M88 の PROPSHEETPAGE より大きいケースに備えている

		int i=0;
		for (PropSheets::iterator n = propsheets.begin(); n != propsheets.end() && i < MAXPROPPAGES; ++n)
		{
			memset(tmppage, 0, sizeof(tmppage));
			tmppage[0].dwSize = sizeof(PROPSHEETPAGE);

			if ((*n)->Setup(this, tmppage))
			{
				memcpy(&psp[i], tmppage, sizeof(PROPSHEETPAGE));
				psp[i].dwSize = sizeof(PROPSHEETPAGE);
				i++;
			}
		}

		if (i > 0)
		{
			memset(&psh, 0, sizeof(psh));
#if _WIN32_IE > 0x200
			psh.dwSize = PROPSHEETHEADER_V1_SIZE;
#else
			psh.dwSize = sizeof(psh);
#endif
			psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS; 
			psh.hwndParent = hwndparent;
			psh.hInstance = hinst;
			psh.pszCaption = "設定";
			psh.nPages = i;
			psh.nStartPage = Min(page, i-1);
			psh.ppsp = psp;
			psh.pfnCallback = (PFNPROPSHEETCALLBACK) (void*)PropProcGate;

			hwndps = (HWND) PropertySheet(&psh);
		}
		delete[] psp;
	}
	else
	{
		SetFocus(hwndps);
	}
	return false;
}

void WinConfig::Close()
{
	if (hwndps)
	{
		DestroyWindow(hwndps);
		hwndps = 0;
	}
}

// ---------------------------------------------------------------------------
//	PropSheetProc
//
bool WinConfig::ProcMsg(MSG& msg)
{
	if (hwndps)
	{
		if (PropSheet_IsDialogMessage(hwndps, &msg))
		{
			if (!PropSheet_GetCurrentPageHwnd(hwndps))
				Close();
			return true;
		}
	}
	return false;
}

int WinConfig::PropProc(HWND hwnd, UINT m, LPARAM)
{
	switch (m)
	{
	case PSCB_INITIALIZED:
		hwndps = hwnd;
		break;
	}
	return 0;
}

int CALLBACK WinConfig::PropProcGate
(HWND hwnd, UINT m, LPARAM l)
{
	if (instance) {
		return instance->PropProc(hwnd, m, l);
	} else {
		return 0;
	}
}

// ---------------------------------------------------------------------------

bool IFCALL WinConfig::Add(IConfigPropSheet* sheet)
{
	propsheets.push_back(sheet);
//	propsheets.insert(propsheets.begin(), sheet);
	return true;
}

bool IFCALL WinConfig::Remove(IConfigPropSheet* sheet)
{
	PropSheets::iterator i = find(propsheets.begin(), propsheets.end(), sheet);
	if (i != propsheets.end())
	{
		propsheets.erase(i);
		return true;
	}
	return false;
}

bool IFCALL WinConfig::PageSelected(IConfigPropSheet* sheet)
{
	PropSheets::iterator i = find(propsheets.begin(), propsheets.end(), sheet);
	if (i == propsheets.end())
	{
		page = 0;
		return false;
	}
	page = i - propsheets.begin();
	return true;
}

bool IFCALL WinConfig::PageChanged(HWND hdlg)
{
	if (hwndps)
		PropSheet_Changed( hwndps, hdlg );
	return true;
}

bool IFCALL WinConfig::Apply()
{
	orgconfig = config;
	PostMessage(hwndparent, WM_M88_APPLYCONFIG, (WPARAM) &config, 0);
	return true;
}

void IFCALL WinConfig::_ChangeVolume(bool current)
{
	PostMessage(hwndparent, 
				WM_M88_CHANGEVOLUME, 
				(WPARAM) (current ? &config : &orgconfig), 
				0);
}

