// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	画面関係 for Windows
// ---------------------------------------------------------------------------
//	$Id: windraw.cpp,v 1.34 2003/04/22 13:16:36 cisc Exp $

#include "headers.h"
#include "misc.h"
#include "windraw.h"
#include "drawgdi.h"
#include "drawdds.h"
#include "drawddw.h"
#include "drawd2d.h"
#include "messages.h"
#include "error.h"
#include "status.h"
#include "loadmon.h"
#include "winexapi.h"

#define LOGNAME "windraw"
#include "diag.h"
//#define DRAW_THREAD

// ---------------------------------------------------------------------------
// 構築/消滅
//
WinDraw::WinDraw()
{
	draw = 0;
	idthread = 0;
	hthread = 0;
	hevredraw = 0;
	drawcount = 0;
	guicount = 0;
	shouldterminate = false;
	drawall = false;
	drawing = false;
	refresh = false;
	locked = false;
	active = false;

}

WinDraw::~WinDraw()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinDraw::Init0(HWND hwindow)
{
	hwnd = hwindow;
	draw = 0;
	drawtype = None;
	
	hthread = 0;
	hevredraw = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	HDC hdc = GetDC(hwnd);
	haspalette = !!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE);
	ReleaseDC(hwnd, hdc);

#ifdef DRAW_THREAD
	if (!hthread)
		hthread = HANDLE(_beginthreadex(NULL, 0, ThreadEntry, 
						   reinterpret_cast<void*> (this), 0, &idthread));
	if (!hthread)
	{
		Error::SetError(Error::ThreadInitFailed);
		return false;
	}
#endif

	palcngbegin = palrgnbegin = 0x100;
	palcngend = palrgnend = -1;

	return true;
}

bool WinDraw::Init(uint w, uint h, uint /*bpp*/)
{
	width = w;
	height = h;
	shouldterminate = false;
	active = true;
	return true;
}

// ---------------------------------------------------------------------------
//	後片付
//
bool WinDraw::Cleanup()
{
	if (hthread)
	{
		SetThreadPriority(hthread, THREAD_PRIORITY_NORMAL);
		
		int i = 300;
		do
		{
			shouldterminate = true;
			SetEvent(hevredraw);
		} while (--i > 0 && WAIT_TIMEOUT == WaitForSingleObject(hthread, 10));
		
		if (!i)
			TerminateThread(hthread, 0);
		
		CloseHandle(hthread), hthread = 0;
	}
	if (hevredraw)
		CloseHandle(hevredraw), hevredraw = 0;
	
	delete draw;	draw = 0;
	return true;
}

// ---------------------------------------------------------------------------
//	画面描画用スレッド
//
uint WinDraw::ThreadMain()
{
	drawing = false;
	while (!shouldterminate)
	{
		PaintWindow();
		WaitForSingleObject(hevredraw, 1000);
	}
	return 0;
}

uint __stdcall WinDraw::ThreadEntry(LPVOID arg)
{
	if (arg)
		return reinterpret_cast<WinDraw*> (arg)->ThreadMain();
	else
		return 0;
}

// ---------------------------------------------------------------------------
//	スレッドの優先順位を下げる
//
void WinDraw::SetPriorityLow(bool low)
{
	if (hthread)
	{
		SetThreadPriority(hthread, 
			low ? THREAD_PRIORITY_BELOW_NORMAL : THREAD_PRIORITY_NORMAL);
	}
}

// ---------------------------------------------------------------------------
//	パレット反映
//
void WinDraw::QueryNewPalette(bool /*bkgnd*/)
{
	CriticalSection::Lock lock(csdraw);
	if (draw)
		draw->QueryNewPalette();
}

// ---------------------------------------------------------------------------
//	ペイントさせる
//
void WinDraw::RequestPaint()
{
#ifdef DRAW_THREAD
	CriticalSection::Lock lock(csdraw);
	drawall = true;
	drawing = true;
	SetEvent(hevredraw);
#else
	drawall = true;
	drawing = true;
	PaintWindow();
#endif
//	LOG1("Request at %d\n", GetTickCount());
}

// ---------------------------------------------------------------------------
//	更新
//
void WinDraw::DrawScreen(const Region& region)
{
	if (!drawing)
	{
		LOG2("Draw %d to %d\n", region.top, region.bottom);
		drawing = true;
		drawarea.left = Max(0, region.left);
		drawarea.top = Max(0, region.top);
		drawarea.right = Min(width, region.right);
		drawarea.bottom = Min(height, region.bottom+1);
#ifdef DRAW_THREAD
		SetEvent(hevredraw);
#else
		PaintWindow();
#endif
	}
}

// ---------------------------------------------------------------------------
//	描画する
//
void WinDraw::PaintWindow()
{
	LOADBEGIN("WinDraw");
	CriticalSection::Lock lock(csdraw);
	if (drawing && draw && active)
	{
		RECT rect = drawarea;
		if (palcngbegin <= palcngend)
		{
			palrgnbegin = Min(palcngbegin, palrgnbegin);
			palrgnend   = Max(palcngend,   palrgnend);
			draw->SetPalette(palette + palcngbegin, palcngbegin, palcngend - palcngbegin + 1);
			palcngbegin = 0x100;
			palcngend = -1;
		}
		draw->DrawScreen(rect, drawall);
		drawall = false;
		if (rect.left < rect.right && rect.top < rect.bottom)
		{
			drawcount++;
			LOG4("\t\t\t(%3d,%3d)-(%3d,%3d)\n", rect.left, rect.top, rect.right-1, rect.bottom-1);
//			statusdisplay.Show(100, 0, "(%.3d, %.3d)-(%.3d, %.3d)", rect.left, rect.top, rect.right-1, rect.bottom-1);
		}
		drawing = false;
	}
	LOADEND("WinDraw");
}

// ---------------------------------------------------------------------------
//	パレットをセット
//
void WinDraw::SetPalette(uint index, uint nents, const Palette* pal)
{
	assert(0 <= index && index <= 0xff);
	assert(index + nents <= 0x100);
	assert(pal);

	memcpy(palette+index, pal, nents * sizeof(Palette));
	palcngbegin = Min(palcngbegin, index);
	palcngend   = Max(palcngend,   index + nents - 1);
}

// ---------------------------------------------------------------------------
//	Lock
//
bool WinDraw::Lock(uint8** pimage, int* pbpl)
{
	assert(pimage && pbpl);

	if (!locked)
	{
		locked = true;
		csdraw.lock();
		if (draw && draw->Lock(pimage, pbpl))
		{
			// Lock に失敗することがある？
			assert (**pimage >= 0);
			return true;
		}
		csdraw.unlock();
		locked = false;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	unlock
//
bool WinDraw::Unlock()
{
	bool result = false;
	if (locked)
	{
		if (draw)
			result = draw->Unlock();
		csdraw.unlock();
		locked = false;
		if (refresh == 1)
			refresh = 0;
	}
	return result;
}

// ---------------------------------------------------------------------------
//	画面サイズを変える
//
void WinDraw::Resize(uint w, uint h)
{
//	statusdisplay.Show(50, 2500, "Resize (%d, %d)", width, height);
	width = w;
	height = h;
	if (draw) {
		csdraw.lock();
		draw->Resize(width, height);
		csdraw.unlock();
	}
}

// ---------------------------------------------------------------------------
//	画面位置を変える
//
void WinDraw::WindowMoved(int x, int y)
{
	if (draw)
		draw->WindowMoved(x, y);
}

// ---------------------------------------------------------------------------
//	画面描画ドライバの変更
//
bool WinDraw::ChangeDisplayMode(bool fullscreen, bool force480)
{
	DisplayType type = fullscreen ? DDFull : D2D;

	// 現在窓(M88)が所属するモニタの GUID を取得
	memset(&gmonitor, 0, sizeof(gmonitor));

	hmonitor = (*MonitorFromWin)(hwnd, MONITOR_DEFAULTTOPRIMARY);
	(*DDEnumerateEx)(DDEnumCallback, reinterpret_cast<LPVOID>(this), DDENUM_ATTACHEDSECONDARYDEVICES);

	if (type != drawtype)
	{
		// 今までのドライバを廃棄
		if (draw)
			draw->SetGUIMode(true);
		{
			CriticalSection::Lock lock(csdraw);
			delete draw; draw = 0;
		}

		// 新しいドライバの用意
		WinDrawSub* newdraw;
		switch (type)
		{
		case GDI: default:
			newdraw = new WinDrawGDI;
			break;
		case DDWin:
			newdraw = new WinDrawDDW;
			break;
		case DDFull:
			newdraw = new WinDrawDDS(force480);
			break;
		case D2D:
			newdraw = new WinDrawD2D;
			break;
		}
		
		bool result = true;
		
		if (!newdraw || !newdraw->Init(hwnd, width, height, &gmonitor))
		{
			// 初期化に失敗した場合 GDI ドライバで再挑戦
			delete newdraw;
			
			if (type == DDFull)
				statusdisplay.Show(50, 2500, "画面切り替えに失敗しました");
			else
				statusdisplay.Show(120, 3000, "GDI ドライバを使用します");

			newdraw = new WinDrawGDI, type = GDI;
			result = false;

			if (!newdraw || !newdraw->Init(hwnd, width, height, 0))
				newdraw = 0, drawtype = None;
		}
		
		if (newdraw)
		{
			newdraw->SetFlipMode(flipmode);
			newdraw->SetGUIMode(false);
		}
		
		// 新しいドライバを使用可能にする
		{
			CriticalSection::Lock lock(csdraw);
			guicount = 0;
			draw = newdraw;
		}

		drawall = true, refresh = true;
		palrgnbegin = Min(palcngbegin, palrgnbegin);
		palrgnend   = Max(palcngend,   palrgnend);
//		if (draw)
//			draw->Resize(width, height);
		
		if (type == DDFull)		ShowCursor(false);
		if (drawtype == DDFull)	ShowCursor(true);
		drawtype = type;

		refresh = 2;
		
		return result;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	現在の状態を得る
//
uint WinDraw::GetStatus()
{
	if (draw)
	{
		if (refresh) refresh = 1; 
		return (!drawing && active ? readytodraw : 0)
			| (refresh ? shouldrefresh : 0)
			| draw->GetStatus();
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	flip モードの設定
//
bool WinDraw::SetFlipMode(bool f)
{
	flipmode = f;
	if (draw)
		return draw->SetFlipMode(flipmode);
	return false;
}

// ---------------------------------------------------------------------------
//	flip する
//
void WinDraw::Flip()
{
	if (draw)
		draw->Flip();
}

// ---------------------------------------------------------------------------
//	GUI 使用フラグ設定
//
void WinDraw::SetGUIFlag(bool usegui)
{
	CriticalSection::Lock lock(csdraw);
	if (usegui)
	{
		if (!guicount++ && draw)
		{
			draw->SetGUIMode(true);
			ShowCursor(true);
		}
	}
	else
	{
		if (!--guicount && draw)
		{
			draw->SetGUIMode(false);
			ShowCursor(false);
		} 
	}
}

// ---------------------------------------------------------------------------
//	画面を 640x400x4 の BMP に変換する
//	dest	変換した BMP の置き場所、置けるだけの領域が必要。
//	ret		巧くできたかどうか
//
int WinDraw::CaptureScreen(uint8* dest)
{
	const bool half = false;
	if (!draw)
		return false;

	uint8* src = new uint8[640*400];
	if (!src) return false;

	uint8* s;
	int bpl;
	if (draw->Lock(&s, &bpl))
	{
		uint8* d = src;
		for (int y=0; y<400; y++)
		{
			memcpy(d, s, 640);
			d+=640, s+=bpl;
		}
		draw->Unlock();
	}

	// 構造体の準備

	BITMAPFILEHEADER* filehdr = (BITMAPFILEHEADER*) dest;
	BITMAPINFO* binfo = (BITMAPINFO*) (filehdr+1);
	
	// headers
	((char*)&filehdr->bfType)[0] = 'B';
	((char*)&filehdr->bfType)[1] = 'M';
	filehdr->bfReserved1 = 0;
	filehdr->bfReserved2 = 0;
	binfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	binfo->bmiHeader.biWidth = 640;
	binfo->bmiHeader.biHeight = half ? 200 : 400;
	binfo->bmiHeader.biPlanes = 1;
	binfo->bmiHeader.biBitCount = 4;
	binfo->bmiHeader.biCompression = BI_RGB;
	binfo->bmiHeader.biSizeImage = 0;
	binfo->bmiHeader.biXPelsPerMeter = 0;
	binfo->bmiHeader.biYPelsPerMeter = 0;

	// １６色パレットの作成
	RGBQUAD* pal = binfo->bmiColors;
	memset(pal, 0, sizeof(RGBQUAD)*16);
	
	uint8 ctable[256];
	memset(ctable, 0, sizeof(ctable));
	
	int colors=0;
	for (int index=0; index<144; index++)
	{
		RGBQUAD rgb;
		rgb.rgbBlue  = palette[0x40+index].peBlue;
		rgb.rgbRed   = palette[0x40+index].peRed;
		rgb.rgbGreen = palette[0x40+index].peGreen;
//		LOG4("c[%.2x] = G:%.2x R:%.2x B:%.2x\n", index, rgb.rgbGreen, rgb.rgbRed, rgb.rgbBlue);
		uint32 entry = *((uint32*)&rgb);
		
		int k;
		for (k=0; k<colors; k++)
		{
			if (!((*((uint32*)&pal[k]) ^ entry) & 0xffffff))
				goto match;
		}
		if (colors<15)
		{
//			LOG4("pal[%.2x] = G:%.2x R:%.2x B:%.2x\n", colors, rgb.rgbGreen, rgb.rgbRed, rgb.rgbBlue);
			pal[colors++] = rgb;
		}
		else 
			k=15;
match:
		ctable[64+index] = k;
	}
	
	binfo->bmiHeader.biClrImportant = colors;
	
	colors = 16;		// やっぱ固定じゃなきゃ駄目か？
	uint8* image = ((uint8*)(binfo+1)) + (colors-1) * sizeof(RGBQUAD);
	filehdr->bfSize	= image + 640*400/2 - dest ;
	binfo->bmiHeader.biClrUsed = colors;
	filehdr->bfOffBits = image - dest;
	
	// 色変換
	uint8* d = image;
	for (int y=0; y<400; y += half ? 2 : 1)
	{
		uint8* s = src + 640 * (399-y);
		
		for (int x=0; x<320; x++, s+=2)
			*d++ = ctable[s[0]] * 16 + ctable[s[1]];
	}

	delete[] src;
	return d - dest;
}

BOOL WINAPI WinDraw::DDEnumCallback
(GUID FAR* guid, LPSTR desc, LPSTR name, LPVOID context, HMONITOR hm)
{
	WinDraw* draw = reinterpret_cast<WinDraw*>(context);
	if (hm == draw->hmonitor)
	{
		draw->gmonitor = *guid;
		return 0;
	}
	return 1;
}

