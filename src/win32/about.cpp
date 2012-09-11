// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	About Dialog Box for M88
// ---------------------------------------------------------------------------
//	$Id: about.cpp,v 1.28 2003/09/28 14:35:35 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "about.h"
#include "version.h"
#include "filetest.h"

// ---------------------------------------------------------------------------
//	構築/消滅
//
M88About::M88About()
{
	dlgproc.SetDestination(DlgProcGate, this);
	
	SanityCheck(&crc);
}

// ---------------------------------------------------------------------------
//	ダイアログ表示
//
void M88About::Show(HINSTANCE hinst, HWND hwndparent)
{
	DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUT), 
				hwndparent, DLGPROC((void*) dlgproc)); 
}

// ---------------------------------------------------------------------------
//	ダイアログ処理
//
BOOL M88About::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	char buf[4096];
	
	switch (msg)
	{
	case WM_INITDIALOG:
		wsprintf(buf, "M88 for Win32 (rel " APP_VER_STRING ")\n"
					  "PC-8801 series emulator.\n"
					  "Copyright (C) 1998, 2003 cisc\n");
	
		SetDlgItemText(hdlg, IDC_ABOUT_TEXT, buf);

		wsprintf(buf, abouttext, crc);

		SetDlgItemText(hdlg, IDC_ABOUT_BOX, buf);

		SetFocus(GetDlgItem(hdlg, IDOK));
		return 0;

	case WM_COMMAND:
		if (IDOK == LOWORD(wp))
		{
			EndDialog(hdlg, true);
			break;
		}
		return true;

	case WM_CLOSE:
		EndDialog(hdlg, false);
		return true;

	default:
		return false;
	}
	return false;
}

BOOL CALLBACK M88About::DlgProcGate
(M88About* about, HWND hwnd, UINT m, WPARAM w, LPARAM l)
{
	return about->DlgProc(hwnd, m, w, l);
}

// ---------------------------------------------------------------------------
//	about 用のテキスト
//
const char M88About::abouttext[] =
	"build date:"__DATE__" (%.8x)\r\n"
	"\r\n"
	"感想・要望・バグ報告などは M88 のページ\r\n"
	"http://www.retropc.net/cisc/m88/\r\n"
	"へどうぞ．\r\n"
	"\r\n"
	"FM 音源ユニットの作成にあたっては，\r\n"
	"佐藤達之氏作の fm.c を参考にさせていただきました．\r\n"
	"\r\n"
	"N80/SR モードは arearea 氏のコードを元に実装されています．\r\n"
	;

