//	$Id: moduleif.cpp,v 1.3 1999/11/26 10:13:09 cisc Exp $

#include "headers.h"
#include "if/ifcommon.h"
#include "if/ifguid.h"
#include "mem.h"
#include "config.h"

#define EXTDEVAPI	__declspec(dllexport)

HINSTANCE hinst;

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

class GRModule : public IModule
{
public:
	GRModule();
	~GRModule() {}

	bool Init(ISystem* system);
	void IFCALL Release();
	void* IFCALL QueryIF(REFIID) { return 0; }

private:
	GVRAMReverse mem;
	ConfigMP cfg;
	
	ISystem* sys;
	IIOBus* bus;
	IConfigPropBase* pb;
};

GRModule::GRModule()
: bus(0), pb(0)
{
	cfg.Init(hinst);
}

bool GRModule::Init(ISystem* _sys)
{
	IMemoryManager* mm;

	sys = _sys;
	
	bus = (IIOBus*) sys->QueryIF(M88IID_IOBus1);
	mm = (IMemoryManager*) sys->QueryIF(M88IID_MemoryManager1);
	pb = (IConfigPropBase*) sys->QueryIF(M88IID_ConfigPropBase);
	
	if (!bus || !mm || !pb)
		return false;
	
	const static IIOBus::Connector c_mp[] =
	{
		{ 0x32, IIOBus::portout, GVRAMReverse::out32 },
		{ 0x35, IIOBus::portout, GVRAMReverse::out35 },
		{ 0x5c, IIOBus::portout, GVRAMReverse::out5x },
		{ 0x5d, IIOBus::portout, GVRAMReverse::out5x },
		{ 0x5e, IIOBus::portout, GVRAMReverse::out5x },
		{ 0x5f, IIOBus::portout, GVRAMReverse::out5x },
		{ 0, 0, 0 }
	};
	if (!mem.Init(mm) || !bus->Connect(&mem, c_mp))
		return false;

	pb->Add(&cfg);
	return true;
}

void GRModule::Release()
{
	if (bus)
		bus->Disconnect(&mem);
	if (pb)
		pb->Remove(&cfg);

	delete this;
}

// ---------------------------------------------------------------------------

//	Module を作成
extern "C" EXTDEVAPI IModule* __cdecl M88CreateModule(ISystem* system)
{
	GRModule* module = new GRModule;
	
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
