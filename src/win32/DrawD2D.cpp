// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	Direct2D+GDI による画面描画 (HiColor 以上)
// ---------------------------------------------------------------------------

#include "headers.h"
#include "drawd2d.h"

#pragma comment( lib,    "delayimp.lib" )
#pragma comment( lib,    "d2d1.lib" )

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinDrawD2D::WinDrawD2D() :
	m_hWnd(0),
	m_hCWnd(0),
	m_image(0),
	m_GDIRT(0),
	m_D2DFact(0),
	m_RenderTarget(0),
	m_UpdatePal(false),
	m_hBitmap(0)
{
}

WinDrawD2D::~WinDrawD2D()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化処理
//
bool WinDrawD2D::Init( HWND _hWnd, uint _width, uint _height, GUID* )
{
	m_hWnd = _hWnd;

	if ( CreateD2D() == false ) {
		return false;
	}

	if ( Resize( _width, _height ) == false ) {
		return false;
	}
	return true;
}

void WinDrawD2D::SetGUIMode( bool _mode )
{
	// mode check

	if ( _mode == 1 ) {
		// Full
	} else {
		// Normal
	}
}


bool WinDrawD2D::CreateD2D()
{
	// OSバージョンのチェック
	OSVERSIONINFOEX osinfo;
	memset( &osinfo, 0, sizeof( osinfo ) );
	osinfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
	::GetVersionEx( (OSVERSIONINFO*)&osinfo );
	if ( osinfo.dwMajorVersion < 6 ) {
		// Vista(ver6.0)以前は失敗
		return false;
	}

	Cleanup();

	if ( m_hCWnd == 0 ) {
		// Base windowを生成
		m_hCWnd = ::CreateWindowEx(
			WS_EX_TRANSPARENT,
			"M88p2 WinUI",
			"",
			WS_CHILD,
			0,
			0,
			640,
			480,
			m_hWnd,
			NULL, NULL, NULL);
	}

	if ( m_hCWnd == 0 ) {
		return false;
	} else {
		::ShowWindow( m_hCWnd, SW_SHOW );
	}

	// D2D factory作成
	HRESULT hr;
	hr = ::D2D1CreateFactory( D2D1_FACTORY_TYPE_MULTI_THREADED,
							  &m_D2DFact );

	if ( SUCCEEDED(hr) ) {
		return true;
	} else {
		return false;
	}
}

//! 画面有効範囲を変更
//
bool WinDrawD2D::Resize( uint _width, uint _height )
{
	m_width  = _width;
	m_height = _height;

	status |= Draw::shouldrefresh;

	HRESULT hr = S_OK;

	::SetWindowPos( m_hCWnd, HWND_BOTTOM, 0, 0, 640, 400, SWP_SHOWWINDOW);

	if ( !m_RenderTarget ) {

		D2D1_SIZE_U size = D2D1::SizeU(
			_width,
			_height
			);

		D2D1_RENDER_TARGET_PROPERTIES RTProps = D2D1::RenderTargetProperties();
		m_D2DFact->GetDesktopDpi( &RTProps.dpiX, &RTProps.dpiY );
		RTProps.usage =  D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

		// Create a GDI compatible Hwnd render target.
		hr = m_D2DFact->CreateHwndRenderTarget(
			RTProps,
			D2D1::HwndRenderTargetProperties( m_hCWnd, size ),
			&m_RenderTarget
			);

		if (SUCCEEDED(hr)) {
			hr = m_RenderTarget->QueryInterface(
				__uuidof(ID2D1GdiInteropRenderTarget),
				(void**)&m_GDIRT
				);
		}

		if ( MakeBitmap() == false ) {
			return false;
		}
		memset( m_image, 0x40, _width * _height);
	} else {
		RECT rc;
		GetClientRect( m_hCWnd, &rc );
		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
			);
		m_RenderTarget->Resize( size );
	}

	if ( !SUCCEEDED(hr) ) {
		return false;
	}


	return true;
}

//! 後片付け
//
bool WinDrawD2D::Cleanup()
{
	SafeRelease( &m_GDIRT );
	SafeRelease( &m_RenderTarget );
	SafeRelease( &m_D2DFact );
	if ( m_hBitmap ) {
		::DeleteObject( m_hBitmap );
		m_hBitmap = 0;
	}
	if ( m_hCWnd ) {
		::DestroyWindow( m_hCWnd );
		m_hCWnd = 0;
	}
	return true;
}

//! BITMAP 作成
//
bool WinDrawD2D::MakeBitmap()
{
	m_bmpinfo.header.biSize          = sizeof(BITMAPINFOHEADER);
	m_bmpinfo.header.biWidth         = m_width;
	m_bmpinfo.header.biHeight        = -(int) m_height;
	m_bmpinfo.header.biPlanes        = 1;
	m_bmpinfo.header.biBitCount      = 8;
	m_bmpinfo.header.biCompression   = BI_RGB;
	m_bmpinfo.header.biSizeImage     = 0;
	m_bmpinfo.header.biXPelsPerMeter = 0;
	m_bmpinfo.header.biYPelsPerMeter = 0;
	m_bmpinfo.header.biClrUsed       = 256;
	m_bmpinfo.header.biClrImportant  = 0;

	m_RenderTarget->BeginDraw();

	// パレットが無い場合
	HDC hdc;
	m_GDIRT->GetDC( D2D1_DC_INITIALIZE_MODE_COPY, &hdc );

	memset( m_bmpinfo.colors, 0, sizeof(RGBQUAD) * 256 );

	if ( m_hBitmap ) {
		::DeleteObject( m_hBitmap );
	}
	BYTE* bitmapimage = 0;
	m_hBitmap = ::CreateDIBSection( hdc,
									(BITMAPINFO*) &m_bmpinfo,
									DIB_RGB_COLORS,
									(void**)(&bitmapimage),
									NULL,
									0 );

	RECT rect = { 0,0,640,400 };
	m_GDIRT->ReleaseDC( &rect );
	m_RenderTarget->EndDraw();

	if ( m_hBitmap == 0 ) {
		return false;
	}

	bpl = m_width;
	m_image = bitmapimage;
	return true;
}

//! パレット設定
//	index 番目のパレットに pe をセット
//
void WinDrawD2D::SetPalette(PALETTEENTRY* _pe, int index, int nentries)
{
	for (; nentries>0; nentries--)
	{
		m_bmpinfo.colors[index].rgbRed   = _pe->peRed;
		m_bmpinfo.colors[index].rgbBlue  = _pe->peBlue;
		m_bmpinfo.colors[index].rgbGreen = _pe->peGreen;
		index++, _pe++;
	}
	m_UpdatePal = true;
}

//! 描画
//
void WinDrawD2D::DrawScreen(const RECT& _rect, bool refresh)
{
	if ( ::IsWindow(m_hWnd) == FALSE ) {
		return;
	}

	m_RenderTarget->BeginDraw();

//	m_RenderTarget->Clear( D2D1::ColorF(D2D1::ColorF::Black) );

	RECT rc = _rect;
	bool valid = rc.left < rc.right && rc.top < rc.bottom;

	if ( refresh || m_UpdatePal ) {
		::SetRect( &rc, 0, 0, m_width, m_height );
		valid = true;
	}

	if (valid) {
		HRESULT hr;
		HDC hDC = NULL;
		hr = m_GDIRT->GetDC( D2D1_DC_INITIALIZE_MODE_COPY, &hDC );

		HDC hmemdc = ::CreateCompatibleDC( hDC );
		HBITMAP oldbitmap = (HBITMAP)::SelectObject( hmemdc, m_hBitmap );
		if ( m_UpdatePal ) {
			m_UpdatePal = false;
			::SetDIBColorTable( hmemdc, 0, 0x100, m_bmpinfo.colors );
		}

		::BitBlt( hDC, rc.left, rc.top,
				  rc.right - rc.left, rc.bottom - rc.top,
				  hmemdc, rc.left, rc.top,
				  SRCCOPY);

		::SelectObject( hmemdc, oldbitmap );
		::DeleteDC( hmemdc );

		RECT rect = { 0,0,640,400 };
		m_GDIRT->ReleaseDC( &rect );
	}

	if ( m_RenderTarget->EndDraw() == D2DERR_RECREATE_TARGET ) {
		// device lost
	}
}

//! 画面イメージの使用要求
// @param _pimage [out] image pointer
// @param _pbpl   [out] width
//
bool WinDrawD2D::Lock(uint8** _pimage, int* _pbpl)
{
	*_pimage = m_image;
	*_pbpl = bpl;
	return m_image != 0;
}

//! 画面イメージの使用終了
//
bool WinDrawD2D::Unlock()
{
	status &= ~Draw::shouldrefresh;
	return true;
}

