// ---------------------------------------------------------------------------
//	M88 - PC88 emulator
//	Copyright (c) cisc 1998.
// ---------------------------------------------------------------------------
//	$Id: messages.h,v 1.4 2001/02/21 11:58:55 cisc Exp $

#pragma once

#define WM_M88_SENDKEYSTATE		(WM_USER+0x200)
#define WM_M88_DISPLAYCHANGED	(WM_USER+0x201)
#define WM_M88_CHANGEDISPLAY	(WM_USER+0x202)
#define WM_M88_APPLYCONFIG		(WM_USER+0x203)	
#define WM_M88_CHANGEVOLUME		(WM_USER+0x204)	
#define WM_M88_CLIPCURSOR		(WM_USER+0x205)

#define CLIPCURSOR_MOUSE	1
#define CLIPCURSOR_WINDOW	2
#define CLIPCURSOR_RELEASE	4

