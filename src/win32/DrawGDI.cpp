// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	GDI による画面描画 (HiColor 以上)
// ---------------------------------------------------------------------------
//	$Id: DrawGDI.cpp,v 1.13 2003/04/22 13:16:35 cisc Exp $

//	bug:パレット～(T-T

#include "headers.h"
#include "drawgdi.h"

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinDrawGDI::WinDrawGDI()
: hwnd(0), hbitmap(0), updatepal(false), bitmapimage(0), image(0)
{
}

WinDrawGDI::~WinDrawGDI()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化処理
//
bool WinDrawGDI::Init(HWND hwindow, uint w, uint h, GUID*)
{
	hwnd = hwindow;

	if (!Resize(w, h))
		return false;
	return true;
}

// ---------------------------------------------------------------------------
//	画面有効範囲を変更
//
bool WinDrawGDI::Resize(uint w, uint h)
{
	width = w;
	height = h;

	if (!MakeBitmap())
		return false;

	memset(image, 0x40, width * height);
	status |= Draw::shouldrefresh;
	return true;
}

// ---------------------------------------------------------------------------
//	後片付け
//
bool WinDrawGDI::Cleanup()
{
	if (hbitmap)
	{
		DeleteObject(hbitmap), hbitmap = 0;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	BITMAP 作成
//
bool WinDrawGDI::MakeBitmap()
{
	binfo.header.biSize          = sizeof(BITMAPINFOHEADER);
	binfo.header.biWidth         = width;
	binfo.header.biHeight        = -(int) height;
	binfo.header.biPlanes        = 1;
	binfo.header.biBitCount      = 8;
	binfo.header.biCompression   = BI_RGB;
	binfo.header.biSizeImage     = 0;
	binfo.header.biXPelsPerMeter = 0;
	binfo.header.biYPelsPerMeter = 0;
	binfo.header.biClrUsed       = 256;
	binfo.header.biClrImportant  = 0;

	// パレットない場合
	
	HDC hdc = GetDC(hwnd);
	memset(binfo.colors, 0, sizeof(RGBQUAD) * 256);

	if (hbitmap)
		DeleteObject(hbitmap);
	hbitmap = CreateDIBSection( hdc,
								(BITMAPINFO*) &binfo,
								DIB_RGB_COLORS,
								(void**)(&bitmapimage),
								NULL, 
								0 );

	ReleaseDC(hwnd, hdc);

	if (!hbitmap)
		return false;
	
	bpl = width;
	image = bitmapimage;
	return true;
}

// ---------------------------------------------------------------------------
//	パレット設定
//	index 番目のパレットに pe をセット
//
void WinDrawGDI::SetPalette(PALETTEENTRY* pe, int index, int nentries)
{
	for (; nentries>0; nentries--)
	{
		binfo.colors[index].rgbRed = pe->peRed;
		binfo.colors[index].rgbBlue = pe->peBlue;
		binfo.colors[index].rgbGreen = pe->peGreen;
		index++, pe++;
	}
	updatepal = true;
}

// ---------------------------------------------------------------------------
//	描画
//
void WinDrawGDI::DrawScreen(const RECT& _rect, bool refresh)
{
	RECT rect = _rect;
	bool valid = rect.left < rect.right && rect.top < rect.bottom;

	if (refresh || updatepal)
		SetRect(&rect, 0, 0, width, height), valid = true;
	
	if (valid)
	{
		HDC hdc = GetDC(hwnd);
		HDC hmemdc = CreateCompatibleDC(hdc);
		HBITMAP oldbitmap = (HBITMAP) SelectObject(hmemdc, hbitmap);
		if (updatepal)
		{
			updatepal = false;
			SetDIBColorTable(hmemdc, 0, 0x100, binfo.colors);
		}
		
		BitBlt(hdc, rect.left, rect.top, 
			        rect.right - rect.left, rect.bottom - rect.top, 
			   hmemdc, rect.left, rect.top, 
			   SRCCOPY);
		
		SelectObject(hmemdc, oldbitmap);
		DeleteDC(hmemdc);
		ReleaseDC(hwnd, hdc);
	}
}

// ---------------------------------------------------------------------------
//	画面イメージの使用要求
//
bool WinDrawGDI::Lock(uint8** pimage, int* pbpl)
{
	*pimage = image;
	*pbpl = bpl;
	return image != 0;
}

// ---------------------------------------------------------------------------
//	画面イメージの使用終了
//
bool WinDrawGDI::Unlock()
{
	status &= ~Draw::shouldrefresh;
	return true;
}

