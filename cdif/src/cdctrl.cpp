// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: cdctrl.cpp,v 1.1 1999/08/26 08:04:36 cisc Exp $

#include "headers.h"
#include "aspi.h"
#include "cdrom.h"
#include "cdctrl.h"
#include "cdromdef.h"

#define LOGNAME "cdctrl"
#include "diag.h"

#define UM_CDCONTROL	(WM_USER + 0x500)

// ---------------------------------------------------------------------------
//	構築・破棄
//
CDControl::CDControl()
{
	hthread = 0;
}

CDControl::~CDControl()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool CDControl::Init(CDROM* cd, Device* dev, DONEFUNC func)
{
	cdrom = cd;
	device = dev;
	donefunc = func;
	diskpresent = false;
	shouldterminate = false;

	if (!hthread)
		hthread = HANDLE(_beginthreadex(NULL, 0, ThreadEntry, 
						   reinterpret_cast<void*> (this), 0, &idthread));
	if (!hthread)
		return false;
	
	while (!PostThreadMessage(idthread, 0, 0, 0))
		Sleep(10);

	vel = 0;
	return true;
}

// ---------------------------------------------------------------------------
//	後片づけ
//
void CDControl::Cleanup()
{
	if (hthread)
	{
		int i=100;
		shouldterminate = true;
		do
		{
			PostThreadMessage(idthread, WM_QUIT, 0, 0);
		} while (WAIT_TIMEOUT == WaitForSingleObject(hthread, 50) && --i);
	
		if (i)
			TerminateThread(hthread, 0);
		
		CloseHandle(hthread);
		hthread = 0;
	}
}

// ---------------------------------------------------------------------------
//	コマンドを実行
//
void CDControl::ExecCommand(uint cmd, uint arg1, uint arg2)
{
	int ret = 0;
	switch (cmd)
	{
	case readtoc:
		ret = cdrom->ReadTOC();
		if (ret)
			diskpresent = true;
		LOG1("Read TOC - %d\n", ret);
		break;

	case playaudio:
		ret = cdrom->PlayAudio(arg1, arg2);
		LOG3("Play Audio(%d, %d) - %d\n", arg1, arg2, ret);
		break;

	case playtrack:
		ret = cdrom->PlayTrack(arg1);
		LOG2("Play Track(%d) - %d\n", arg1, ret);
		break;

	case stop:
		ret = cdrom->Stop();
		LOG1("Stop - %d\n", ret);
		if (arg1)
			return;
		break;

	case pause:
		ret = cdrom->Pause(true);
		LOG1("Pause - %d\n", ret);
		break;

	case checkdisk:
		ret = cdrom->CheckMedia();
		LOG1("CheckDisk - %d\n", ret);
		if (diskpresent)
		{
			if (!ret)
				diskpresent = false;
			LOG0("unmount\n");
		}
		else
		{
			if (ret)
			{
				if (cdrom->ReadTOC())
					diskpresent = true;
				LOG0("Mount\n");
			}
		}
		break;

	case readsubcodeq:
		ret = cdrom->ReadSubCh((uint8*) arg1, true);
		break;

	case read1:
		ret = cdrom->Read(arg1, (uint8*) arg2, 2048);
		break;

	case read2:
		ret = cdrom->Read2(arg1, (uint8*) arg2, 2340);
		break;

	default:
		return;
	}
	if (donefunc && !shouldterminate)
		(device->*donefunc)(ret);
	return;
}

// ---------------------------------------------------------------------------
//	現在の時間を求める
//
uint CDControl::GetTime()
{
	if (diskpresent)
	{
		SubcodeQ sub;
		cdrom->ReadSubCh((uint8*) &sub, false);

		return sub.absaddr;
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	コマンドを送る
//
bool CDControl::SendCommand(uint cmd, uint arg1, uint arg2)
{
	return !!PostThreadMessage(idthread, UM_CDCONTROL + cmd, arg1, arg2);
}

// ---------------------------------------------------------------------------
//	スレッド
//
uint CDControl::ThreadMain()
{
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) && !shouldterminate)
	{
		if (UM_CDCONTROL <= msg.message && msg.message < UM_CDCONTROL + ncmds)
			ExecCommand(msg.message - UM_CDCONTROL, msg.wParam, msg.lParam);
	}
	return 0;
}

uint __stdcall CDControl::ThreadEntry(LPVOID arg)
{
	if (arg)
		return reinterpret_cast<CDControl*> (arg)->ThreadMain();
	else
		return 0;
}
