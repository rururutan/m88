//
//	Windows 系 includes
//	すべての標準ヘッダーを含む
//
//	$Id: headers.h,v 1.13 2003/05/12 22:26:35 cisc Exp $
//

#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN

#define DIRECTSOUND_VERSION	0x500	// for pre-DirectX7 environment

#define FORW2K					// W2K 用の SDK を使用

#ifdef FORW2K
	#define WINVER			0x500	// for Win2000
	#define _WIN32_WINNT	0x500
#endif

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <dsound.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <process.h>
#include <assert.h>
#include <time.h>

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include "types.h"

using namespace std;

// --- STL 関係

#ifdef _MSC_VER
	#undef max
	#define max _MAX
	#undef min
	#define min _MIN
#endif

// --- OPENFILENAME 関係

#if _WIN32_WINNT < 0x500
	struct OFNV5 : public OPENFILENAME
	{
		void* pvReserved;
		DWORD dwReserved;
		DWORD FlagsEx;
	};
	#define OFNV4SIZE			sizeof(OPENFILENAME)
	#define OFN_EX_NOPLACESBAR	0x00000001
#else
	#define OFNV5				OPENFILENAME
	#define OFNV4SIZE			OPENFILENAME_SIZE_VERSION_400
#endif

