// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	User Interface for Win32
// ---------------------------------------------------------------------------
//	$Id: ui.h,v 1.33 2002/05/31 09:45:21 cisc Exp $

#ifndef win_ui_h
#define win_ui_h

#include "types.h"
#include "instthnk.h"
#include "wincore.h"
#include "WinDraw.h"
#include "WinKeyIF.h"
#include "88config.h"
#include "wincfg.h"
#include "newdisk.h"
#include "soundmon.h"
#include "memmon.h"
#include "codemon.h"
#include "basmon.h"
#include "regmon.h"
#include "loadmon.h"
#include "iomon.h"

// ---------------------------------------------------------------------------

class WinUI
{
public:
	WinUI(HINSTANCE hinst);
	~WinUI();
	
	bool InitWindow(int nwinmode);
	int  Main(const char* cmdline);
	uint GetMouseButton() { return mousebutton; }
	HWND GetHWnd() { return hwnd; }

private:
	struct DiskInfo
	{
		HMENU hmenu;
		int currentdisk;
		int idchgdisk;
		bool readonly;
		char filename[MAX_PATH];
	};
	struct ExtNode
	{
		ExtNode* next;
		IWinUIExtention* ext;
	};

private:
	bool InitM88(const char* cmdline);
	void CleanupM88();
	LRESULT WinProc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK WinProcGate(WinUI*, HWND, UINT, WPARAM, LPARAM);
	void ReportError();
	void Reset();

	void ApplyConfig();
	void ApplyCommandLine(const char* cmdline);

	void ToggleDisplayMode();
	void ChangeDisplayType(bool savepos);

	void ChangeDiskImage(HWND hwnd, int drive);
	bool OpenDiskImage(int drive, const char* filename, bool readonly, int id, bool create);
	void OpenDiskImage(const char* filename);
	bool SelectDisk(uint drive, int id, bool menuonly);
	bool CreateDiskMenu(uint drive);

	void OpenTape();

	void ShowStatusWindow();
	void ResizeWindow(uint width, uint height);
	void SetGUIFlag(bool);

	void SaveSnapshot(int n);
	void LoadSnapshot(int n);
	void GetSnapshotName(char*, int);
	bool MakeSnapshotMenu();

	void CaptureScreen();

	int AllocControlID();
	void FreeControlID(int);

	// ウインドウ関係
	InstanceThunk winproc;
	HWND hwnd;
	HINSTANCE hinst;
	HACCEL accel;
	HMENU hmenudbg;

	// 状態表示用
	UINT timerid;
	bool report;
	volatile bool active;

	// ウインドウの状態
	bool background;
	bool fullscreen;
	uint displaychangedtime;
	uint resetwindowsize;
	DWORD wstyle;
	POINT point;
	int clipmode;
	bool guimodebymouse;
	
	// disk
	DiskInfo diskinfo[2];
	char tapetitle[MAX_PATH];
	
	// snapshot 関係
	HMENU hmenuss[2];
	int currentsnapshot;
	bool snapshotchanged;

	// PC88
	bool capturemouse;
	uint mousebutton;
	
	WinCore core;
	WinDraw draw;
	PC8801::WinKeyIF keyif;
	PC8801::Config config;
	PC8801::WinConfig winconfig;
	WinNewDisk newdisk;
	OPNMonitor opnmon;
	PC8801::MemoryMonitor memmon;
	PC8801::CodeMonitor codemon;
	PC8801::BasicMonitor basmon;
	PC8801::IOMonitor iomon;
	Z80RegMonitor regmon;
	LoadMonitor loadmon;
	DiskManager* diskmgr;
	TapeManager* tapemgr;

private:
	// メッセージ関数
	uint M88ChangeDisplay(HWND, WPARAM, LPARAM);
	uint M88ChangeVolume(HWND, WPARAM, LPARAM);
	uint M88ApplyConfig(HWND, WPARAM, LPARAM);
	uint M88SendKeyState(HWND, WPARAM, LPARAM);
	uint M88ClipCursor(HWND, WPARAM, LPARAM);
	uint WmDropFiles(HWND, WPARAM, LPARAM);
	uint WmDisplayChange(HWND, WPARAM, LPARAM);
	uint WmKeyDown(HWND, WPARAM, LPARAM);
	uint WmKeyUp(HWND, WPARAM, LPARAM);
	uint WmSysKeyDown(HWND, WPARAM, LPARAM);
	uint WmSysKeyUp(HWND, WPARAM, LPARAM);
	uint WmInitMenu(HWND, WPARAM, LPARAM);
	uint WmQueryNewPalette(HWND, WPARAM, LPARAM);
	uint WmPaletteChanged(HWND, WPARAM, LPARAM);
	uint WmActivate(HWND, WPARAM, LPARAM);
	uint WmTimer(HWND, WPARAM, LPARAM);
	uint WmClose(HWND, WPARAM, LPARAM);
	uint WmCreate(HWND, WPARAM, LPARAM);
	uint WmDestroy(HWND, WPARAM, LPARAM);
	uint WmPaint(HWND, WPARAM, LPARAM);
	uint WmCommand(HWND, WPARAM, LPARAM);
	uint WmSize(HWND, WPARAM, LPARAM);
	uint WmDrawItem(HWND, WPARAM, LPARAM);
	uint WmEnterMenuLoop(HWND, WPARAM, LPARAM);
	uint WmExitMenuLoop(HWND, WPARAM, LPARAM);
	uint WmLButtonDown(HWND, WPARAM, LPARAM);
	uint WmLButtonUp(HWND, WPARAM, LPARAM);
	uint WmRButtonDown(HWND, WPARAM, LPARAM);
	uint WmRButtonUp(HWND, WPARAM, LPARAM);
	uint WmEnterSizeMove(HWND, WPARAM, LPARAM);
	uint WmExitSizeMove(HWND, WPARAM, LPARAM);
	uint WmMove(HWND, WPARAM, LPARAM);
	uint WmMouseMove(HWND, WPARAM, LPARAM);
	uint WmSetCursor(HWND, WPARAM, LPARAM);
};

#endif // WIN_UI_H
