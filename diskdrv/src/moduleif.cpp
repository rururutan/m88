//	$Id: moduleif.cpp,v 1.2 1999/11/26 10:12:57 cisc Exp $

#include "headers.h"
#include "if/ifcommon.h"
#include "if/ifguid.h"
#include "diskio.h"

#define EXTDEVAPI	__declspec(dllexport)

using namespace PC8801;

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

class DiskDrvModule : public IModule
{
public:
	DiskDrvModule();
	~DiskDrvModule() {}

	bool Init(ISystem* system);
	void IFCALL Release();
	void* IFCALL QueryIF(REFIID) { return 0; }

private:
	DiskIO diskio;
	
	ISystem* sys;
	IIOBus* bus;
};

DiskDrvModule::DiskDrvModule()
: diskio(0)
{
}

bool DiskDrvModule::Init(ISystem* _sys)
{
	sys = _sys;
	bus = (IIOBus*) sys->QueryIF(M88IID_IOBus1);
	if (!bus) return false;

	const static IIOBus::Connector c_diskio[] =
	{
		{ pres, IIOBus::portout, DiskIO::reset },
		{ 0xd0, IIOBus::portout, DiskIO::setcommand },
		{ 0xd1, IIOBus::portout, DiskIO::setdata },
		{ 0xd0, IIOBus::portin,  DiskIO::getstatus },
		{ 0xd1, IIOBus::portin,  DiskIO::getdata },
		{ 0, 0, 0 }
	};
	if (!diskio.Init() || !bus->Connect(&diskio, c_diskio))
		return false;
	
	return true;
}

void DiskDrvModule::Release()
{
	if (bus)
		bus->Disconnect(&diskio);
	delete this;
}

// ---------------------------------------------------------------------------

//	Module を作成
extern "C" EXTDEVAPI IModule* __cdecl M88CreateModule(ISystem* system)
{
	DiskDrvModule* module = new DiskDrvModule;
	
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
