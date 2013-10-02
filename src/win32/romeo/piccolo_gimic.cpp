/**
 * PICCOLO for G.I.M.I.C
 *
 * License : GNU General Public License
 */

#include "headers.h"
#include <windows.h>
#include "piccolo.h"
#include "piccolo_gimic.h"
#include "c86ctl.h"
#include "misc.h"
#include "status.h"

#define LOGNAME "piccolo"
#include "diag.h"

HMODULE			g_mod = 0;
IRealChipBase	*g_chipbase = 0;

struct GMCDRV
{
	IGimic *gimic;
	IRealChip *chip;
	int	type;
	bool reserve;
};

static GMCDRV gmcdrv = { 0, 0, PICCOLO_INVALID, false };

class GimicIf : public PiccoloChip
{
  public:
	GimicIf(Piccolo* p) :  pic(p) {}
	~GimicIf() {
		Log("YMF288::Reset()\n");
		Mute();
	}
	int	 Init(uint param) {
		gmcdrv.chip->reset();
		return true;
	}
	void Reset(bool opna) {
		pic->DrvReset();
		gmcdrv.chip->reset();
		gmcdrv.gimic->setSSGVolume( opna ? 63 : 68 );	// FM/PSG”ä 50%/55%
	}
	bool SetReg( uint32 at, uint addr, uint data ) {
		if (gmcdrv.chip) {
			gmcdrv.chip->out( addr, data );
		}
//		pic->DrvSetReg( at, addr, data );
		return true;
	}
	void SetChannelMask(uint mask){};
	void SetVolume(int ch, int value){};
  private:
	Piccolo* pic;
//	Piccolo::Driver* drv;
  private:
	void Mute() {
		Log("YMF288::Mute()\n");
		gmcdrv.chip->out( 0x07, 0x3f );
		for (uint r=0x40; r<0x4f; r++) {
			if (~r & 3)
			  {
				  gmcdrv.chip->out(0x000 + r, 0x7f);
				  gmcdrv.chip->out(0x100 + r, 0x7f);
			  }
		}
	}
};

static bool LoadDLL()
{
	if ( g_mod == NULL ) {
		g_mod = ::LoadLibrary( "c86ctl.dll" );
		if ( !g_mod ) {
			return false;
		}

		typedef HRESULT (WINAPI *TCreateInstance)( REFIID, LPVOID* );
		TCreateInstance pCI;
		pCI = (TCreateInstance)::GetProcAddress( g_mod, "CreateInstance" );

		if ( pCI ) {
			(*pCI)( IID_IRealChipBase, (void**)&g_chipbase );
			if (g_chipbase) {
				g_chipbase->initialize();
			} else {
				::FreeLibrary( g_mod );
				g_mod = 0;
			}
		} else {
			::FreeLibrary( g_mod );
			g_mod = 0;
		}
	}
	return g_mod ? true : false;
}

static void FreeDLL()
{
	if ( g_chipbase ) {
		g_chipbase->deinitialize();
		g_chipbase = 0;
	}
	if ( g_mod ) {
		::FreeLibrary( g_mod );
		g_mod = 0;
	}
}

// ---------------------------------------------------------------------------
//
//
Piccolo_Gimic::Piccolo_Gimic() :
	Piccolo()
{
}

Piccolo_Gimic::~Piccolo_Gimic()
{
	Cleanup();
	FreeDLL();
}

int Piccolo_Gimic::Init()
{
	// DLL —pˆÓ
	Log("LoadDLL\n");
	if (!LoadDLL()) {
		return PICCOLOE_DLL_NOT_FOUND;
	}

	// OPN3/OPNA‚Ì‘¶ÝŠm”F
	Log("FindDevice\n");
	int devnum = g_chipbase->getNumberOfChip();
	bool found_device = false;
	for (int i=0; i < devnum; i++) {
		g_chipbase->getChipInterface( i, IID_IGimic, (void**)&gmcdrv.gimic );
		g_chipbase->getChipInterface( i, IID_IRealChip, (void**)&gmcdrv.chip );

		Devinfo info;
		gmcdrv.gimic->getModuleInfo( &info );

		if ( strncmp( info.Devname, "GMC-OPNA", sizeof(info.Devname) ) == 0 ) {
			gmcdrv.type = PICCOLO_YM2608;
			found_device = true;
			gmcdrv.chip->reset();
			gmcdrv.gimic->setPLLClock( 7987200 );
			gmcdrv.gimic->setSSGVolume( 63 );	// FM/PSG”ä 50%
			avail = 2;
			break;
		}
		if ( strncmp( info.Devname, "GMC-OPN3L", sizeof(info.Devname) ) == 0 ) {
			gmcdrv.type = PICCOLO_YMF288;
			found_device = true;
			gmcdrv.chip->reset();
			avail = 1;
			break;
		}
	}

	if ( found_device == false ) {
		return PICCOLOE_ROMEO_NOT_FOUND;
	}

	Piccolo::Init();

	return PICCOLO_SUCCESS;
}

int Piccolo_Gimic::GetChip(PICCOLO_CHIPTYPE _type, PiccoloChip** _pc)
{
	*_pc = 0;
	Log("GetChip %d\n", _type);

	if ( !avail ) {
		return PICCOLOE_HARDWARE_NOT_AVAILABLE;
	}

	if ( _type != PICCOLO_YMF288 && _type != PICCOLO_YM2608 ) {
		return PICCOLOE_HARDWARE_NOT_AVAILABLE;
	}

	Log(" Type: YMF288\n");
	if ( gmcdrv.type == PICCOLO_INVALID ) {
		return PICCOLOE_HARDWARE_NOT_AVAILABLE;
	}
	if ( gmcdrv.reserve == true ) {
		return PICCOLOE_HARDWARE_IN_USE;
	}

	Log(" allocated\n");
	gmcdrv.reserve = true;
	*_pc = new GimicIf(this);

	return PICCOLO_SUCCESS;
}

void Piccolo_Gimic::SetReg( uint addr, uint data ) {
	if ( avail ) {
		gmcdrv.chip->out( addr, data );
	}
}
