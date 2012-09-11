// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	GDI ‚É‚æ‚é‰æ–Ê•`‰æ
// ---------------------------------------------------------------------------
//	$Id: DrawGDI.h,v 1.6 2001/02/21 11:58:53 cisc Exp $

#if !defined(win32_drawgdi_h)
#define win32_drawgdi_h

// ---------------------------------------------------------------------------

#include "windraw.h"

class WinDrawGDI : public WinDrawSub
{
public:
	WinDrawGDI();
	~WinDrawGDI();

	bool Init(HWND hwnd, uint w, uint h, GUID*);
	bool Resize(uint width, uint height);
	bool Cleanup();
	void SetPalette(PALETTEENTRY* pal, int index, int nentries);
	void DrawScreen(const RECT& rect, bool refresh);
	bool Lock(uint8** pimage, int* pbpl);
	bool Unlock();

private:
	struct BI256		// BITMAPINFO
	{
		BITMAPINFOHEADER header;
		RGBQUAD colors[256];
	};

private:
	bool	MakeBitmap();
	
	HBITMAP	hbitmap;
	uint8*	bitmapimage;
	HWND	hwnd;
	uint8*	image; 
	int		bpl;
	uint	width;
	uint	height;
	bool	updatepal;
	BI256	binfo;
};

#endif // !defined(win32_drawgdi_h)
