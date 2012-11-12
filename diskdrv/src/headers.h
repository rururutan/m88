//
//	Windows 系 includes
//	すべての標準ヘッダーを含む
//
//	$Id: headers.h,v 1.3 1999/10/10 16:05:52 cisc Exp $
//

#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE		0x200

#include <windows.h>
#include <objbase.h>
#include <commdlg.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <process.h>
#include <assert.h>
#include <malloc.h>

#ifdef _MSC_VER
	#undef max
	#define max _MAX
	#undef min
	#define min _MIN
#endif

