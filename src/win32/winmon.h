// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: winmon.h,v 1.4 2002/04/07 05:40:11 cisc Exp $

#pragma once

//	概要：
//	デバッグ表示のために使用するテキストウィンドウの管理用基本クラス
//	機能：
//	テキストバッファ，差分更新，スクロールバーサポート

#include "types.h"

// ---------------------------------------------------------------------------

class WinMonitor
{
public:
	WinMonitor();
	~WinMonitor();

	bool Init(LPCTSTR tmpl);
	void Show(HINSTANCE, HWND, bool show);
	
	bool IsOpen() { return !!hwnd; }
	
	void Update();

protected:
	void Locate(int x, int y);
	void Puts(const char*);
	void Putf(const char*, ...);

	void SetTxCol(COLORREF col) { txcol = col; }
	void SetBkCol(COLORREF col) { bkcol = col; }
	
	int  GetWidth() { return width; }
	int  GetHeight() { return height; }
	int  GetLine() { return line; }
	void SetLine(int l);
	void SetLines(int nlines);
	bool GetTextPos(POINT*);
	bool EnableStatus(bool);
	bool PutStatus(const char* text, ...);
	
	bool SetFont(HWND hwnd, int fh);
	
	void ClearText();
	
	int  GetScrPos(bool tracking);
	void ScrollUp();
	void ScrollDown();
	
	void SetUpdateTimer(int t);
	
	virtual void DrawMain(HDC, bool = false);
	virtual BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	
	HWND GetHWnd() { return hwnd; }
	HWND GetHWndStatus() { return hwndstatus; }
	HINSTANCE GetHInst() { return hinst; }

private:
	virtual void UpdateText();
	virtual int VerticalScroll(int msg);
	virtual void Start() {}
	virtual void Stop() {}

	static BOOL CALLBACK DlgProcGate(HWND, UINT, WPARAM, LPARAM);
	
	void Draw(HWND, HDC);
	void ResizeWindow(HWND);

	HWND hwnd;
	HINSTANCE hinst;
	LPCTSTR lptemplate;

	HWND hwndstatus;
	char statusbuf[128];

	int clientwidth;
	int clientheight;
	
	HDC hdc;
	HFONT hfont;
	int fontwidth;
	int fontheight;

	HBITMAP hbitmap;
	
	struct TXCHAR
	{
		char ch;
		COLORREF txcol;
		COLORREF bkcol;
	};

	TXCHAR* txtbuf;
	TXCHAR* txpbuf;
	TXCHAR* txtbufptr;
	POINT txp;
	int width;
	int height;

	COLORREF txcol, txcolprev;
	COLORREF bkcol, bkcolprev;

	RECT wndrect;

	int line;
	int nlines;

	UINT_PTR timer;
	int timerinterval;

};

