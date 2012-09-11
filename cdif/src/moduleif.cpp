//	$Id: moduleif.cpp,v 1.2 1999/11/26 10:12:48 cisc Exp $

#include "headers.h"
#include "if/ifcommon.h"
#include "if/ifguid.h"
#include "cdif.h"
#include "config.h"

#define EXTDEVAPI	__declspec(dllexport)

using namespace PC8801;

static HINSTANCE hinst;

// ---------------------------------------------------------------------------

enum SpecialPort
{
	pint0 = 0x100, 
	pint1, pint2, pint3, pint4, pint5, pint6, pint7,
	pres,			// reset
	pirq,			// IRQ
	piack,			// interrupt acknowledgement
	vrtc,			// vertical retrace
	popnio,			// OPN の入出力ポート 1
	popnio2,		// OPN の入出力ポート 2 (連番)
	portend
};

// ---------------------------------------------------------------------------

class CDROMModule : public IModule
{
public:
	CDROMModule();
	~CDROMModule() {}

	bool Init(ISystem* system);
	void IFCALL Release();
	void* IFCALL QueryIF(REFIID) { return 0; }

private:
	CDIF cdif;
	ConfigCDIF cfg;
	
	ISystem* sys;
	IIOBus* bus;
	IConfigPropBase* pb;
};

CDROMModule::CDROMModule()
: cdif(DEV_ID('c', 'd', 'i', 'f'))
{
	cfg.Init(hinst);
}

bool CDROMModule::Init(ISystem* _sys)
{
	IDMAAccess* dmac;

	sys = _sys;
	
	bus = (IIOBus*) sys->QueryIF(M88IID_IOBus1);
	dmac = (IDMAAccess*) sys->QueryIF(M88IID_DMA);
	pb = (IConfigPropBase*) sys->QueryIF(M88IID_ConfigPropBase);
	if (!bus || !dmac || !pb) return false;

	const static IIOBus::Connector c_cdif[] =
	{
		{ pres,  IIOBus::portout, CDIF::reset },
		{ 0x90,  IIOBus::portout, CDIF::out90 },
		{ 0x91,  IIOBus::portout, CDIF::out91 },
		{ 0x94,  IIOBus::portout, CDIF::out94 },
		{ 0x97,  IIOBus::portout, CDIF::out97 },
		{ 0x98,  IIOBus::portout, CDIF::out98 },
		{ 0x99,  IIOBus::portout, CDIF::out99 },
		{ 0x9f,  IIOBus::portout, CDIF::out9f },
		{ 0x90,  IIOBus::portin,  CDIF::in90 },
		{ 0x91,  IIOBus::portin,  CDIF::in91 },
		{ 0x92,  IIOBus::portin,  CDIF::in92 },
		{ 0x93,  IIOBus::portin,  CDIF::in93 },
		{ 0x96,  IIOBus::portin,  CDIF::in96 },
		{ 0x98,  IIOBus::portin,  CDIF::in98 },
		{ 0x99,  IIOBus::portin,  CDIF::in99 },
		{ 0x9b,  IIOBus::portin,  CDIF::in9b },
		{ 0x9d,  IIOBus::portin,  CDIF::in9d },
		{ 0, 0, 0 }
	};
	if (!cdif.Init(dmac) || !bus->Connect(&cdif, c_cdif))
		return false;
	cdif.Enable(true);
	pb->Add(&cfg);
	
	return true;
}

void CDROMModule::Release()
{
	if (bus)
		bus->Disconnect(&cdif);
	if (pb)
		pb->Remove(&cfg);
	delete this;
}

// ---------------------------------------------------------------------------

//	Module を作成
extern "C" EXTDEVAPI IModule* __cdecl M88CreateModule(ISystem* system)
{
	CDROMModule* module = new CDROMModule;
	
	if (module)
	{
		if (module->Init(system))
			return module;
		delete module;
	}
	return 0;
}

BOOL APIENTRY DllMain(HANDLE hmod, DWORD rfc, LPVOID)
{
	switch (rfc)
	{
	case DLL_PROCESS_ATTACH:
		hinst = (HINSTANCE) hmod;
		break;
	
	case DLL_THREAD_ATTACH:
		break;
	
	case DLL_THREAD_DETACH:
		break;
	
	case DLL_PROCESS_DETACH:
		break;
	}
	return true;
}
