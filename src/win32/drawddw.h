// ---------------------------------------------------------------------------
//  M88 - PC8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	DirectDraw によるウインドウ画面描画
// ---------------------------------------------------------------------------
//	$Id: drawddw.h,v 1.7 2002/04/07 05:40:10 cisc Exp $

#if !defined(win32_drawddw_h)
#define win32_drawddw_h

#include "windraw.h"

// ---------------------------------------------------------------------------

class WinDrawDDW : public WinDrawSub
{
public:
	WinDrawDDW();
	~WinDrawDDW();

	bool Init(HWND hwnd, uint w, uint h, GUID*);
	bool Resize(uint width, uint height);
	bool Cleanup();
	
	void SetPalette(PALETTEENTRY* pal, int index, int nentries);
	void QueryNewPalette();
	void DrawScreen(const RECT& rect, bool refresh);
	bool Lock(uint8** pimage, int* pbpl);
	bool Unlock();

private:
	bool CreateDDPalette();
	bool CreateDD2();
	bool CreateDDSPrimary();
	bool CreateDDSWork();
	bool RestoreSurface();
	HWND hwnd;

	static void Convert8bpp(void* dest, const uint8* src, RECT* rect, int pitch);
	
	LPDIRECTDRAW2 ddraw;
	LPDIRECTDRAWPALETTE ddpal;
	
	LPDIRECTDRAWSURFACE ddsprimary;
	LPDIRECTDRAWSURFACE ddsscrn;
	LPDIRECTDRAWCLIPPER ddcscrn;
	
	LPDIRECTDRAWSURFACE ddswork;
	
	uint32 redmask;
	uint32 greenmask;
	uint32 bluemask;
	uint8 redshift;
	uint8 greenshift;
	uint8 blueshift;
	bool scrnhaspal;
	bool palchanged;
	bool locked;

	uint width;
	uint height;

	PALETTEENTRY palentry[256];
};

#endif // !defined(win32_drawddw_h)
