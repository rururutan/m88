// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: cfgpage.h,v 1.3 2002/05/15 21:38:02 cisc Exp $

#pragma once

#include "if/ifcommon.h"
#include "pc88/config.h"

namespace PC8801
{

class ConfigPage : public IConfigPropSheet
{
public:
	ConfigPage(Config& c, Config& oc);
	bool Init(HINSTANCE _hinst);  
	bool IFCALL Setup(IConfigPropBase*, PROPSHEETPAGE* psp);
	
private:
	virtual LPCSTR GetTemplate() = 0;
	virtual void InitDialog(HWND hdlg) {}
	virtual void Update(HWND hdlg) {}
	virtual void UpdateSlider(HWND hdlg) {}
	virtual void SetActive(HWND hdlg) {}
	virtual bool Clicked(HWND hdlg, HWND hwctl, UINT id) { return false; }
	virtual BOOL Command(HWND hdlg, HWND hwctl, UINT nc, UINT id) { return false; }
	virtual void Apply(HWND hdlg) {}

	BOOL PageProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK PageGate(HWND, UINT, WPARAM, LPARAM);

	HINSTANCE hinst;

protected:
	IConfigPropBase* base;
	Config& config;
	Config& orgconfig;
};


class ConfigCPU : public ConfigPage
{
public:
	ConfigCPU(Config& c, Config& oc) : ConfigPage(c, oc) {}

private:
	LPCSTR GetTemplate();
	void InitDialog(HWND hdlg);
	void SetActive(HWND hdlg);
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void Update(HWND hdlg);
	void UpdateSlider(HWND hdlg);
	BOOL Command(HWND hdlg, HWND hwctl, UINT nc, UINT id);
};

class ConfigScreen : public ConfigPage
{
public:
	ConfigScreen(Config& c, Config& oc) : ConfigPage(c, oc) {}

private:
	LPCSTR GetTemplate();
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void Update(HWND hdlg);
};

class ConfigSound : public ConfigPage
{
public:
	ConfigSound(Config& c, Config& oc) : ConfigPage(c, oc) {}

private:
	LPCSTR GetTemplate();
	void InitDialog(HWND hdlg);
	void SetActive(HWND hdlg);
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void Update(HWND hdlg);
	BOOL Command(HWND hdlg, HWND hwctl, UINT nc, UINT id);
};

class ConfigVolume : public ConfigPage
{
public:
	ConfigVolume(Config& c, Config& oc) : ConfigPage(c, oc) {}

private:
	LPCSTR GetTemplate();
	void InitDialog(HWND hdlg);
	void SetActive(HWND hdlg);
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void UpdateSlider(HWND hdlg);
	void Apply(HWND hdlg);

	static void InitVolumeSlider(HWND hdlg, UINT id, int val);
	static void SetVolumeText(HWND hdlg, int id, int val);
};


class ConfigFunction : public ConfigPage
{
public:
	ConfigFunction(Config& c, Config& oc) : ConfigPage(c, oc) {}

private:
	LPCSTR GetTemplate();
	void InitDialog(HWND hdlg);
	void SetActive(HWND hdlg);
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void Update(HWND hdlg);
	void UpdateSlider(HWND hdlg);
};

class ConfigSwitch : public ConfigPage
{
public:
	ConfigSwitch(Config& c, Config& oc) : ConfigPage(c, oc) {}

private:
	LPCSTR GetTemplate();
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void Update(HWND hdlg);
};

class ConfigEnv : public ConfigPage
{
public:
	ConfigEnv(Config& c, Config& oc) : ConfigPage(c, oc) {}

private:
	LPCSTR GetTemplate();
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void Update(HWND hdlg);
};

class ConfigROMEO : public ConfigPage
{
public:
	ConfigROMEO(Config& c, Config& oc) : ConfigPage(c, oc) {}

private:
	LPCSTR GetTemplate();
	void InitDialog(HWND hdlg);
	void SetActive(HWND hdlg);
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void UpdateSlider(HWND hdlg);
	void Apply(HWND hdlg);

	static void InitSlider(HWND hdlg, UINT id, int val);
	static void SetText(HWND hdlg, int id, int val);
};


}

