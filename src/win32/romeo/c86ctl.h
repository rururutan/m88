/***
	c86ctl
	
	Copyright (c) 2009-2010, honet. All rights reserved.
	This software is licensed under the BSD license.

	honet.kk(at)gmail.com
 */


#ifndef _C86CTL_H
#define _C86CTL_H

#include <ObjBase.h>

#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/*  定数定義                                                                  */
/*----------------------------------------------------------------------------*/
#define C86CTL_ERR_NONE						0
#define C86CTL_ERR_UNKNOWN					-1
#define C86CTL_ERR_INVALID_PARAM			-2
#define C86CTL_ERR_NOT_IMPLEMENTED			-9999
#define C86CTL_ERR_NODEVICE					-1000

enum ChipType {
	CHIP_UNKNOWN = 0,
	CHIP_OPNA,
	CHIP_OPM,
	CHIP_OPN3L,
};

/*----------------------------------------------------------------------------*/
/*  構造体定義                                                                */
/*----------------------------------------------------------------------------*/
struct Devinfo{
	char Devname[16];
	char Rev;
	char Serial[15];
};

/*----------------------------------------------------------------------------*/
/*  Interface定義                                                             */
/*----------------------------------------------------------------------------*/
// IRealChipBase {5C457918-E66D-4AC1-8CB5-B91C4704DF79}
static const GUID IID_IRealChipBase = 
{ 0x5c457918, 0xe66d, 0x4ac1, { 0x8c, 0xb5, 0xb9, 0x1c, 0x47, 0x4, 0xdf, 0x79 } };

interface IRealChipBase : public IUnknown
{
	virtual int __stdcall initialize(void) = 0;
	virtual int __stdcall deinitialize(void) = 0;
	virtual int __stdcall getNumberOfChip(void) = 0;
	virtual HRESULT __stdcall getChipInterface( int id, REFIID riid, void** ppi ) = 0;
};


// IRealChip {F959C007-6B4D-46F3-BB60-9B0897C7E642}
static const GUID IID_IRealChip = 
{ 0xf959c007, 0x6b4d, 0x46f3, { 0xbb, 0x60, 0x9b, 0x8, 0x97, 0xc7, 0xe6, 0x42 } };

interface IRealChip : public IUnknown
{
public:
	virtual int __stdcall reset(void) = 0;
	virtual void __stdcall out( UINT addr, UCHAR data ) = 0;
	virtual UCHAR __stdcall in( UINT addr ) = 0;
	//virtual __stdcall getModuleType() = 0;
};


// IGimic {175C7DA0-8AA5-4173-96DA-BB43B8EB8F17}
static const GUID IID_IGimic = 
{ 0x175c7da0, 0x8aa5, 0x4173, { 0x96, 0xda, 0xbb, 0x43, 0xb8, 0xeb, 0x8f, 0x17 } };

interface IGimic : public IUnknown
{
	virtual int __stdcall getFWVer( UINT *major, UINT *minor, UINT *revision, UINT *build ) = 0;
	virtual int __stdcall getMBInfo( struct Devinfo *info ) = 0;
	virtual int __stdcall getModuleInfo( struct Devinfo *info ) = 0;
	virtual int __stdcall setSSGVolume(UCHAR vol) = 0;
	virtual int __stdcall getSSGVolume(UCHAR *vol) = 0;
	virtual int __stdcall setPLLClock(UINT clock) = 0;
	virtual int __stdcall getPLLClock(UINT *clock) = 0;
};



/*----------------------------------------------------------------------------*/
/*  公開関数定義                                                              */
/*----------------------------------------------------------------------------*/
HRESULT WINAPI CreateInstance( REFIID riid, void** ppi );


int WINAPI c86ctl_initialize(void);					// DEPRECATED
int WINAPI c86ctl_deinitialize(void);				// DEPRECATED
int WINAPI c86ctl_reset(void);						// DEPRECATED
void WINAPI c86ctl_out( UINT addr, UCHAR data );	// DEPRECATED
UCHAR WINAPI c86ctl_in( UINT addr );				// DEPRECATED



#ifdef __cplusplus
}
#endif

#endif
