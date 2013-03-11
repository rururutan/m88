// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	Direct2D ‚É‚æ‚é‰æ–Ê•`‰æ
// ---------------------------------------------------------------------------

#pragma once

// ---------------------------------------------------------------------------

#include <d2d1.h>
#include <d2d1helper.h>
#include "windraw.h"

class WinDrawD2D : public WinDrawSub
{
public:
	WinDrawD2D();
	~WinDrawD2D();

	bool Init(HWND hwnd, uint w, uint h, GUID*);
	bool Resize(uint width, uint height);
	bool Cleanup();
	void SetPalette(PALETTEENTRY* pal, int index, int nentries);
	void SetGUIMode(bool guimode);
	void DrawScreen(const RECT& rect, bool refresh);
	bool Lock(uint8** pimage, int* pbpl);
	bool Unlock();

private:
	bool CreateD2D();

	struct BI256		// BITMAPINFO
	{
		BITMAPINFOHEADER header;
		RGBQUAD colors[256];
	};

	bool	MakeBitmap();

	ID2D1Factory *m_D2DFact;
	ID2D1HwndRenderTarget *m_RenderTarget;
	ID2D1GdiInteropRenderTarget *m_GDIRT;

	bool	m_UpdatePal;
	HWND	m_hWnd;
	HWND	m_hCWnd;
	uint	m_width;
	uint	m_height;
	BYTE*	m_image; 	// ‰æ‘œBitmap
	BI256	m_bmpinfo;
	HBITMAP	m_hBitmap;
	int		bpl;
};
