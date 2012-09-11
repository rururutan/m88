// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998.
// ---------------------------------------------------------------------------
//	DirectDraw によるウインドウ画面描画
//	8bpp 専用(T-T
// ---------------------------------------------------------------------------
//	$Id: drawddw.cpp,v 1.11 2002/04/07 05:40:10 cisc Exp $

#include "headers.h"
#include "drawddw.h"
#include "misc.h"

#define LOGNAME "drawddw"
#include "diag.h"
#include "dderr.h"

#define RELCOM(x)  if (x) x->Release(), x=0; else 0

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinDrawDDW::WinDrawDDW()
{
	ddraw = 0;
	ddcscrn = 0;
	ddpal = 0;
	ddswork = 0;
	ddsprimary = 0;
	scrnhaspal = false;
	palchanged = false;
	locked = false;
}

WinDrawDDW::~WinDrawDDW()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinDrawDDW::Init(HWND hwindow, uint w, uint h, GUID*)
{
	hwnd = hwindow;

	width = w;
	height = h;

	if (!CreateDD2())
		return false;

	HRESULT hr = ddraw->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
	LOGDDERR("DirectDraw::SetCooperativeLevel()", hr);
	if (hr != DD_OK)
		return false;
	
	CreateDDPalette();

	if (!Resize(w, h))
		return false;
	return true;
}

bool WinDrawDDW::Resize(uint w, uint h)
{
	width = w;
	height = h;
	
	RELCOM(ddcscrn);
	RELCOM(ddswork);
	RELCOM(ddsprimary);
	
	if (!CreateDDSPrimary() || !CreateDDSWork())
		return false;
	
	DDPIXELFORMAT ddpf;
	memset(&ddpf, 0, sizeof(ddpf));
	ddpf.dwSize = sizeof(DDPIXELFORMAT);
	if (DD_OK != ddsscrn->GetPixelFormat(&ddpf))
		return false;
	if (!(ddpf.dwFlags & DDPF_RGB))
		return false;
	scrnhaspal = !!(ddpf.dwFlags & DDPF_PALETTEINDEXED8);
	if (ddpf.dwRGBBitCount != 8)
		return false;
	
	status |= Draw::shouldrefresh;
	return true;
}

// ---------------------------------------------------------------------------
//	Cleanup
//
bool WinDrawDDW::Cleanup()
{
	RELCOM(ddpal);
	RELCOM(ddcscrn);
	RELCOM(ddswork);
	RELCOM(ddsprimary);
	RELCOM(ddraw);
	return true;
}

// ---------------------------------------------------------------------------
//	DirectDraw2 準備
//
bool WinDrawDDW::CreateDD2()
{
	if (FAILED(CoCreateInstance(CLSID_DirectDraw, 0, CLSCTX_ALL, IID_IDirectDraw2, (void**) &ddraw)))
		return false;
	if (FAILED(ddraw->Initialize(0)))
		return false;
	return true;
}

// ---------------------------------------------------------------------------
//	Primary Surface 作成
//
bool WinDrawDDW::CreateDDSPrimary()
{
	HRESULT hr;
	// 表示サーフェスを作成
	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	RELCOM(ddsprimary);
	hr = ddraw->CreateSurface(&ddsd, &ddsprimary, 0);
	LOGDDERR("DirectDraw::CreateSurface - primary", hr);
	if (hr != DD_OK)
		return false;
	
	// クリッパーをつける
	RELCOM(ddcscrn);
	hr = ddraw->CreateClipper(0, &ddcscrn, 0);
	LOGDDERR("DirectDraw::CreateClipper()", hr);
	if (hr != DD_OK)
		return false;

	ddcscrn->SetHWnd(0, hwnd);
	ddsprimary->SetClipper(ddcscrn);

	ddsscrn = ddsprimary;

	ddsprimary->SetPalette(ddpal);
	return true;
}

// ---------------------------------------------------------------------------
//	作業用サーフェスを作成
//
bool WinDrawDDW::CreateDDSWork()
{
	HRESULT hr;
	DDSURFACEDESC ddsd;
	
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
	ddsd.ddpfPixelFormat.dwRGBBitCount = 8;
	
	RELCOM(ddswork);
	hr = ddraw->CreateSurface(&ddsd, &ddswork, 0);
	LOGDDERR("DirectDraw::CreateSurface - work", hr);
	if (hr != DD_OK)
		return false;
	return true;
}

// ---------------------------------------------------------------------------
//	パレット準備
//
bool WinDrawDDW::CreateDDPalette()
{
	int i;
	const int nsyscol = 10;
	for (i=0; i<nsyscol; i++)
	{
		palentry[i].peRed = i;
		palentry[i].peGreen = 0;
		palentry[i].peBlue = 0;
		palentry[i].peFlags = PC_EXPLICIT;
		palentry[255-i].peRed = 255-i;
		palentry[255-i].peGreen = 0;
		palentry[255-i].peBlue = 0;
		palentry[255-i].peFlags = PC_EXPLICIT;
	}
	for (i=nsyscol; i<0x40; i++)
	{
		palentry[i].peRed = 0;
		palentry[i].peGreen = 0;
		palentry[i].peBlue = 0;
		palentry[i].peFlags = PC_NOCOLLAPSE;
	}
	if (scrnhaspal)
	{
		HRESULT hr = ddraw->CreatePalette(DDPCAPS_8BIT, palentry, &ddpal, 0);
		LOGDDERR("DirectDraw::CreatePalette()", hr);
	}
	return true;
}

// ---------------------------------------------------------------------------
//	描画
//
void WinDrawDDW::DrawScreen(const RECT& _rect, bool refresh)
{
	RECT rect = _rect;

	if (ddpal && palchanged)
	{
		palchanged = false;
		HRESULT hr = ddpal->SetEntries(0, 0, 0x100, palentry);
		LOGDDERR("DirectDrawPalette::SetEntries()", hr);
	}

	if (refresh)
		rect.left = 0, rect.right = width, rect.top = 0, rect.bottom = height;

	// 作業領域を更新
	if (rect.top < rect.bottom)
	{
		HRESULT hr;
		POINT pos;
		pos.x = pos.y = 0;
		ClientToScreen(hwnd, &pos);

		RECT rectdest;
		rectdest.left = pos.x + rect.left;
		rectdest.right = pos.x + rect.right;
		rectdest.top = pos.y + rect.top;
		rectdest.bottom = pos.y + rect.bottom;
		hr = ddsscrn->Blt(&rectdest, ddswork, &rect, DDBLT_WAIT, 0);
		
		LOGDDERR("DirectDrawSurface::Blt()", hr);
		if (hr == DDERR_SURFACELOST) 
			RestoreSurface();
	}
}

// ---------------------------------------------------------------------------
//	WM_QUERYNEWPALETTE
//
void WinDrawDDW::QueryNewPalette()
{
	if (scrnhaspal)
	{
		HRESULT hr = ddsscrn->SetPalette(ddpal);
		LOGDDERR("DirectDrawSurface::SetPalette - scrn", hr);
		hr = ddswork->SetPalette(ddpal);
		LOGDDERR("DirectDrawSurface::SetPalette - work", hr);
	}
}

// ---------------------------------------------------------------------------
//	パレットを設定
//
void WinDrawDDW::SetPalette(PALETTEENTRY* pe, int i, int n)
{
	for (; n>0; n--)
	{
		palentry[i].peRed   = pe->peRed;
		palentry[i].peBlue  = pe->peBlue;
		palentry[i].peGreen = pe->peGreen;
		palentry[i].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
		i++, pe++;
	}
	palchanged = true;
}

// ---------------------------------------------------------------------------
//	画面イメージの使用要求
//
bool WinDrawDDW::Lock(uint8** pimage, int* pbpl)
{
	if (!locked)
	{
		locked = true;

		RECT rect;
		rect.left = 0, rect.top = 0;
		rect.right = width, rect.bottom = height;

		DDSURFACEDESC ddsd;
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		HRESULT hr = ddswork->Lock(&rect, &ddsd, 0, 0);
		LOGDDERR("DirectDrawSurface::Lock()", hr);
		if (hr != DD_OK)
			return false;

		*pimage = (uint8*) ddsd.lpSurface;
		*pbpl = ddsd.lPitch;

		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	画面イメージの使用終了
//
bool WinDrawDDW::Unlock()
{
	if (locked)
	{
		HRESULT hr = ddswork->Unlock(0);
		LOGDDERR("DirectDrawSurface::Unlock()", hr);
		status &= ~Draw::shouldrefresh;
		locked = false;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	ロストしたサーフェスを戻す
//
bool WinDrawDDW::RestoreSurface()
{
	if (DD_OK != ddsscrn->Restore() || DD_OK != ddswork->Restore())
		return false;

	status |= Draw::shouldrefresh;
	return true;
}
