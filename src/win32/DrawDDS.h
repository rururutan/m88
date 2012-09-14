// ---------------------------------------------------------------------------
//  M88 - PC8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	DirectDraw(32/24/16bpp) ‚É‚æ‚é‘S‰æ–Ê•`‰æ
// ---------------------------------------------------------------------------
//	$Id: DrawDDS.h,v 1.10 2002/04/07 05:40:10 cisc Exp $

#pragma once
#include "windraw.h"

// ---------------------------------------------------------------------------

class WinDrawDDS : public WinDrawSub
{
public:
	WinDrawDDS(bool force480 = true);
	~WinDrawDDS();

	bool Init(HWND hwnd, uint w, uint h, GUID* drv);
	bool Resize(uint w, uint h);
	bool Cleanup();

	void SetPalette(PALETTEENTRY* pal, int index, int nentries);
	void QueryNewPalette();
	void DrawScreen(const RECT& rect, bool refresh);
	bool Lock(uint8** pimage, int* pbpl);
	bool Unlock();
	void SetGUIMode(bool guimode);
	void Flip();
	bool SetFlipMode(bool f);

private:
	void FillBlankArea();
	bool RestoreSurface();
	bool SetScreenMode();
	bool CreateDD2(GUID* monitor);
	bool CreateDDS();
	bool Update(LPDIRECTDRAWSURFACE, const RECT& rect);

	static HRESULT WINAPI EDMCallBack(LPDDSURFACEDESC, LPVOID);

	HWND hwnd;

	LPDIRECTDRAW2 ddraw;
	LPDIRECTDRAWSURFACE ddsscrn;
	LPDIRECTDRAWSURFACE ddsback;
	LPDIRECTDRAWCLIPPER ddcscrn;
	
	uint lines;		// 400 or 480
	uint width;
	uint height;
	POINT ltc;
	
	uint8* image;
	
	bool palchanged;
	bool guimode;
	int m_bpp;

	PALETTEENTRY palentry[256];
};
