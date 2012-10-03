// ---------------------------------------------------------------------------
//	M88 - PC88 emulator
//	Copyright (c) cisc 1998. 1999.
// ---------------------------------------------------------------------------
//	PC88 Keyboard Interface Emulation for Win32/106 key (Rev. 3)
// ---------------------------------------------------------------------------
//	$Id: WinKeyIF.cpp,v 1.8 2000/02/04 01:50:00 cisc Exp $

#include "headers.h"
#include "WinKeyIF.h"
#include "messages.h"
#include "pc88/config.h"
#include "misc.h"

//#define LOGNAME "keyif"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	Construct/Destruct
//
WinKeyIF::WinKeyIF()
: Device(0)
{
	hwnd = 0;
	hevent = 0;
	for (int i=0; i<16; i++)
	{
		keyport[i] = -1;
	}
	memset(keyboard, 0, 256);
	memset(keystate, 0, 512);
	usearrow = false;

	disable = false;
}

WinKeyIF::~WinKeyIF()
{
	if (hevent)
		CloseHandle(hevent);
}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinKeyIF::Init(HWND hwndmsg)
{
	hwnd = hwndmsg;
	hevent = CreateEvent(0, 0, 0, 0);
	keytable = KeyTable106[0];
	return hevent != 0;
}

// ---------------------------------------------------------------------------
//	リセット（というか、BASIC モードの変更）
//
void IOCALL WinKeyIF::Reset(uint, uint)
{
	pc80mode = (basicmode & 2) != 0; 
}

// ---------------------------------------------------------------------------
//	設定反映
//
void WinKeyIF::ApplyConfig(const Config* config)
{
	usearrow = 0 != (config->flags & Config::usearrowfor10);
	basicmode = config->basicmode;

	switch (config->keytype)
	{
	case Config::PC98:
		keytable = KeyTable98[0];
		break;

	case Config::AT101:
		keytable = KeyTable101[0];
		break;

	case Config::AT106:
	default:
		keytable = KeyTable106[0];
		break;
	}
}

 // ---------------------------------------------------------------------------
//	WM_KEYDOWN
//
void WinKeyIF::KeyDown(uint vkcode, uint32 keydata)
{
	if (keytable == KeyTable106[0])
	{
		// 半角・全角キー対策
		if (vkcode == 0xf3 || vkcode == 0xf4)
		{
			keystate[0xf4] = 3;
			return;
		}
	}
	uint keyindex = (vkcode & 0xff) | ((keydata & (1<<24)) ? 0x100 : 0);
	LOG2("KeyDown  = %.2x %.3x\n", vkcode, keyindex);
	keystate[keyindex] = 1;
}

// ---------------------------------------------------------------------------
//	WM_KEYUP
//
void WinKeyIF::KeyUp(uint vkcode, uint32 keydata)
{
	uint keyindex = (vkcode & 0xff) | (keydata & (1<<24) ? 0x100 : 0);
	keystate[keyindex] = 0;
	LOG2("KeyUp   = %.2x %.3x\n", vkcode, keyindex);
	
	// SHIFT + テンキーによる押しっぱなし現象対策
	
	if (keytable == KeyTable106[0] || keytable == KeyTable101[0])
	{
		switch (keyindex)
		{
		case VK_NUMPAD0: case VK_INSERT:
			keystate[VK_NUMPAD0] = keystate[VK_INSERT] = 0;
			break;
		case VK_NUMPAD1: case VK_END:
			keystate[VK_NUMPAD1] = keystate[VK_END] = 0;
			break;
		case VK_NUMPAD2: case VK_DOWN:
			keystate[VK_NUMPAD2] = keystate[VK_DOWN] = 0;
			break;
		case VK_NUMPAD3: case VK_NEXT:
			keystate[VK_NUMPAD3] = keystate[VK_NEXT] = 0;
			break;
		case VK_NUMPAD4: case VK_LEFT:
			keystate[VK_NUMPAD4] = keystate[VK_LEFT] = 0;
			break;
		case VK_NUMPAD5: case VK_CLEAR:
			keystate[VK_NUMPAD5] = keystate[VK_CLEAR] = 0;
			break;
		case VK_NUMPAD6: case VK_RIGHT:
			keystate[VK_NUMPAD6] = keystate[VK_RIGHT] = 0;
			break;
		case VK_NUMPAD7: case VK_HOME:
			keystate[VK_NUMPAD7] = keystate[VK_HOME] = 0;
			break;
		case VK_NUMPAD8: case VK_UP:
			keystate[VK_NUMPAD8] = keystate[VK_UP] = 0;
			break;
		case VK_NUMPAD9: case VK_PRIOR:
			keystate[VK_NUMPAD9] = keystate[VK_PRIOR] = 0;
			break;
		}
	}
}

// ---------------------------------------------------------------------------
//	Key
//	keyboard によるキーチェックは反応が鈍いかも知れず
//
uint WinKeyIF::GetKey(const Key* key)
{
	uint i;

	for (i=0; i<8 && key->k; i++, key++)
	{
		switch (key->f)
		{
		case lock:
			if (keyboard[key->k] & 0x01)
				return 0;
			break;

		case keyb:
			if (keyboard[key->k] & 0x80)
				return 0;
			break;
		
		case nex:
			if (keystate[key->k])
				return 0;
			break;

		case ext:
			if (keystate[key->k | 0x100])
				return 0;
			break;

		case arrowten:
			if (usearrow && (keyboard[key->k] & 0x80))
				return 0;
			break;

		case noarrowten:
			if (!usearrow && (keyboard[key->k] & 0x80))
				return 0;
			break;

		case noarrowtenex:
			if (!usearrow && keystate[key->k | 0x100])
				return 0;
			break;

		case pc80sft:
			if (pc80mode && ((keyboard[VK_DOWN] & 0x80) || (keyboard[VK_LEFT] & 0x80)))
				return 0;
			break;

		case pc80key:
			if (pc80mode && (keyboard[key->k] & 0x80))
				return 0;
			break;

		default:
			if (keystate[key->k] | keystate[key->k | 0x100]) // & 0x80)
				return 0;
			break;
		}
	}
	return 1;
}

// ---------------------------------------------------------------------------
//	VSync 処理
//
void IOCALL WinKeyIF::VSync(uint,uint d)
{
	if (d && active)
	{
		if (hwnd)
		{
			PostMessage(hwnd, WM_M88_SENDKEYSTATE, 
				reinterpret_cast<DWORD>(keyboard), (DWORD) hevent);
			WaitForSingleObject(hevent, 10);
		}

		if (keytable == KeyTable106[0] || keytable == KeyTable101[0])
		{
			keystate[0xf4] = Max(keystate[0xf4]-1, 0);
		}
		for (int i=0; i<16; i++)
		{
			keyport[i] = -1;
		}
	}
}

void WinKeyIF::Activate(bool yes)
{
	active = yes;
	if (active)
	{
		memset(keystate, 0, 512);
		for (int i=0; i<16; i++)
		{
			keyport[i] = -1;
		}
	}
}

void WinKeyIF::Disable(bool yes)
{
	disable = yes;
}

// ---------------------------------------------------------------------------
//	キー入力
//
uint IOCALL WinKeyIF::In(uint port)
{
	port &= 0x0f;
	
	if (active)
	{
		int r = keyport[port];
		if (r == -1)
		{
			const Key* key = keytable + port * 64 + 56;
			r=0;
			for (int i=0; i<8; i++)
			{
				r = r * 2 + GetKey(key);
				key -= 8;
			}
			keyport[port] = r;
			if (port == 0x0d)
			{	
				LOG3("In(13)   = %.2x %.2x %.2x\n", r, keystate[0xf4], keystate[0x1f4]);
			}
		}
		return uint8(r);
	}
	else
		return 0xff;
}

// ---------------------------------------------------------------------------
//	キー対応表
//	ひとつのキーに書けるエントリ数は８個まで。
//	８個未満の場合は最後に TERM を付けること。
//	
//	KEYF の f は次のどれか。
//	nex		WM_KEYxxx の extended フラグが 0 のキーのみ
//	ext		WM_KEYxxx の extended フラグが 1 のキーのみ
//	lock	ロック機能を持つキー (別に CAPS LOCK やカナのように物理的
//								  ロック機能を持っている必要は無いはず)
//	arrowten 方向キーをテンキーに対応させる場合のみ
//

#define KEY(k)     { k, 0 }
#define KEYF(k,f)  { k, f }
#define TERM       { 0, 0 }

// ---------------------------------------------------------------------------
//	キー対応表 for 日本語 106 キーボード
//
const WinKeyIF::Key WinKeyIF::KeyTable106[16 * 8][8] =
{
	// 00
	{ KEY(VK_NUMPAD0), KEYF(VK_INSERT, nex), TERM, },	// num 0
	{ KEY(VK_NUMPAD1), KEYF(VK_END,	   nex), TERM, },	// num 1
	{ KEY(VK_NUMPAD2), KEYF(VK_DOWN,   nex), KEYF(VK_DOWN, arrowten), TERM, },
	{ KEY(VK_NUMPAD3), KEYF(VK_NEXT,   nex), TERM, },	// num 3
	{ KEY(VK_NUMPAD4), KEYF(VK_LEFT,   nex), KEYF(VK_LEFT, arrowten), TERM, },
	{ KEY(VK_NUMPAD5), KEYF(VK_CLEAR,  nex), TERM, },	// num 5
	{ KEY(VK_NUMPAD6), KEYF(VK_RIGHT,  nex), KEYF(VK_RIGHT,arrowten), TERM, },
	{ KEY(VK_NUMPAD7), KEYF(VK_HOME,   nex), TERM, },	// num 7
	
	// 01
	{ KEY(VK_NUMPAD8), KEYF(VK_UP,     nex), KEYF(VK_UP  , arrowten), TERM, },
	{ KEY(VK_NUMPAD9), KEYF(VK_PRIOR,  nex), TERM, },	// num 9
	{ KEY(VK_MULTIPLY), TERM, },						// num *
	{ KEY(VK_ADD),		TERM, },						// num +
	{ TERM, },											// num =
	{ KEY(VK_SEPARATOR), KEYF(VK_DELETE, nex), TERM, },	// num ,
	{ KEY(VK_DECIMAL),	TERM, },						// num .
	{ KEY(VK_RETURN),	TERM, },						// RET

	// 02
	{ KEY(0xc0),TERM },	// @
	{ KEY('A'),	TERM }, // A
	{ KEY('B'),	TERM }, // B
	{ KEY('C'),	TERM }, // C
	{ KEY('D'),	TERM }, // D
	{ KEY('E'),	TERM }, // E
	{ KEY('F'),	TERM }, // F
	{ KEY('G'),	TERM }, // G

	// 03
	{ KEY('H'),	TERM }, // H
	{ KEY('I'),	TERM }, // I
	{ KEY('J'),	TERM }, // J
	{ KEY('K'),	TERM }, // K
	{ KEY('L'),	TERM }, // L
	{ KEY('M'),	TERM }, // M
	{ KEY('N'),	TERM }, // N
	{ KEY('O'),	TERM }, // O

	// 04
	{ KEY('P'),	TERM }, // P
	{ KEY('Q'),	TERM }, // Q
	{ KEY('R'),	TERM }, // R
	{ KEY('S'),	TERM }, // S
	{ KEY('T'),	TERM }, // T
	{ KEY('U'),	TERM }, // U
	{ KEY('V'),	TERM }, // V
	{ KEY('W'),	TERM }, // W

	// 05
	{ KEY('X'),	TERM }, // X
	{ KEY('Y'),	TERM }, // Y
	{ KEY('Z'),	TERM }, // Z
	{ KEY(0xdb),TERM }, // [
	{ KEY(0xdc),TERM }, // \ 
	{ KEY(0xdd),TERM }, // ]
	{ KEY(0xde),TERM }, // ^
	{ KEY(0xbd),TERM }, // -

	// 06
	{ KEY('0'),	TERM }, // 0
	{ KEY('1'),	TERM }, // 1
	{ KEY('2'),	TERM }, // 2
	{ KEY('3'),	TERM }, // 3
	{ KEY('4'),	TERM }, // 4
	{ KEY('5'),	TERM }, // 5
	{ KEY('6'),	TERM }, // 6
	{ KEY('7'),	TERM }, // 7

	// 07
	{ KEY('8'),	TERM }, // 8
	{ KEY('9'),	TERM }, // 9
	{ KEY(0xba),TERM }, // :
	{ KEY(0xbb),TERM }, // ;
	{ KEY(0xbc),TERM }, // ,
	{ KEY(0xbe),TERM }, // .
	{ KEY(0xbf),TERM }, // /
	{ KEY(0xe2),TERM }, // _

	// 08
	{ KEYF(VK_HOME, ext),	TERM }, // CLR
	{ KEYF(VK_UP, noarrowtenex),	KEYF(VK_DOWN, pc80key), TERM }, // ↑
	{ KEYF(VK_RIGHT, noarrowtenex),	KEYF(VK_LEFT, pc80key), TERM }, // →
	{ KEY(VK_BACK),	KEYF(VK_INSERT, ext), KEYF(VK_DELETE, ext), TERM }, // BS
	{ KEY(VK_MENU),			TERM }, // GRPH
	{ KEYF(VK_SCROLL, lock),TERM }, // カナ
	{ KEY(VK_SHIFT), KEY(VK_F6), KEY(VK_F7), KEY(VK_F8), KEY(VK_F9), KEY(VK_F10), KEYF(VK_INSERT, ext), KEYF(1, pc80sft) }, // SHIFT
	{ KEY(VK_CONTROL),		TERM }, // CTRL

	// 09
	{ KEY(VK_F11),KEY(VK_PAUSE), TERM }, // STOP
	{ KEY(VK_F1), KEY(VK_F6),	TERM }, // F1
	{ KEY(VK_F2), KEY(VK_F7),	TERM }, // F2
	{ KEY(VK_F3), KEY(VK_F8),	TERM }, // F3
	{ KEY(VK_F4), KEY(VK_F9),	TERM }, // F4
	{ KEY(VK_F5), KEY(VK_F10),	TERM }, // F5
	{ KEY(VK_SPACE), KEY(VK_CONVERT), KEY(VK_NONCONVERT), TERM }, // SPACE
	{ KEY(VK_ESCAPE),	TERM }, // ESC

	// 0a
	{ KEY(VK_TAB),			TERM }, // TAB
	{ KEYF(VK_DOWN, noarrowtenex),	TERM }, // ↓
	{ KEYF(VK_LEFT, noarrowtenex),	TERM }, // ←
	{ KEYF(VK_END, ext), KEY(VK_HELP), TERM }, // HELP
	{ KEY(VK_F12), TERM }, // COPY
	{ KEY(0x6d),			TERM }, // -
	{ KEY(0x6f),			TERM }, // /
	{ KEYF(VK_CAPITAL, lock), TERM }, // CAPS LOCK

	// 0b
	{ KEYF(VK_NEXT, ext),	TERM }, // ROLL DOWN
	{ KEYF(VK_PRIOR, ext),	TERM }, // ROLL UP
	{ TERM, },
	{ TERM, },
	{ TERM, },
	{ TERM, },
	{ TERM, },
	{ TERM, },

	// 0c
	{ KEY(VK_F6), TERM },			// F6
	{ KEY(VK_F7), TERM },			// F7
	{ KEY(VK_F8), TERM },			// F8
	{ KEY(VK_F9), TERM },			// F9
	{ KEY(VK_F10), TERM },			// F10
	{ KEY(VK_BACK), TERM },			// BS
	{ KEYF(VK_INSERT, ext), TERM },	// INS
	{ KEYF(VK_DELETE, ext), TERM },	// DEL

	// 0d
	{ KEY(VK_CONVERT), TERM },		// 変換
	{ KEY(VK_NONCONVERT), KEY(VK_ACCEPT), TERM }, // 決定
	{ TERM },						// PC
	{ KEY(0xf4), TERM },			// 全角
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },

	// 0e
	{ KEYF(VK_RETURN, nex), TERM },		// RET FK
	{ KEYF(VK_RETURN, ext), TERM },		// RET 10
	{ KEY(VK_LSHIFT), TERM },		// SHIFT L
	{ KEY(VK_RSHIFT), TERM },		// SHIFT R
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },

	// 0f
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
};


// ---------------------------------------------------------------------------
//	キー対応表 for 101 キーボード
//
const WinKeyIF::Key WinKeyIF::KeyTable101[16 * 8][8] =
{
	// 00
	{ KEY(VK_NUMPAD0), KEYF(VK_INSERT, nex), TERM, },	// num 0
	{ KEY(VK_NUMPAD1), KEYF(VK_END,	   nex), TERM, },	// num 1
	{ KEY(VK_NUMPAD2), KEYF(VK_DOWN,   nex), KEYF(VK_DOWN, arrowten), TERM, },
	{ KEY(VK_NUMPAD3), KEYF(VK_NEXT,   nex), TERM, },	// num 3
	{ KEY(VK_NUMPAD4), KEYF(VK_LEFT,   nex), KEYF(VK_LEFT, arrowten), TERM, },
	{ KEY(VK_NUMPAD5), KEYF(VK_CLEAR,  nex), TERM, },	// num 5
	{ KEY(VK_NUMPAD6), KEYF(VK_RIGHT,  nex), KEYF(VK_RIGHT,arrowten), TERM, },
	{ KEY(VK_NUMPAD7), KEYF(VK_HOME,   nex), TERM, },	// num 7
	
	// 01
	{ KEY(VK_NUMPAD8), KEYF(VK_UP,     nex), KEYF(VK_UP  , arrowten), TERM, },
	{ KEY(VK_NUMPAD9), KEYF(VK_PRIOR,  nex), TERM, },	// num 9
	{ KEY(VK_MULTIPLY), TERM, },						// num *
	{ KEY(VK_ADD),		TERM, },						// num +
	{ TERM, },											// num =
	{ KEY(VK_SEPARATOR), KEYF(VK_DELETE, nex), TERM, },	// num ,
	{ KEY(VK_DECIMAL),	TERM, },						// num .
	{ KEY(VK_RETURN),	TERM, },						// RET

	// 02
	{ KEY(0xdb),TERM },	// @
	{ KEY('A'),	TERM }, // A
	{ KEY('B'),	TERM }, // B
	{ KEY('C'),	TERM }, // C
	{ KEY('D'),	TERM }, // D
	{ KEY('E'),	TERM }, // E
	{ KEY('F'),	TERM }, // F
	{ KEY('G'),	TERM }, // G

	// 03
	{ KEY('H'),	TERM }, // H
	{ KEY('I'),	TERM }, // I
	{ KEY('J'),	TERM }, // J
	{ KEY('K'),	TERM }, // K
	{ KEY('L'),	TERM }, // L
	{ KEY('M'),	TERM }, // M
	{ KEY('N'),	TERM }, // N
	{ KEY('O'),	TERM }, // O

	// 04
	{ KEY('P'),	TERM }, // P
	{ KEY('Q'),	TERM }, // Q
	{ KEY('R'),	TERM }, // R
	{ KEY('S'),	TERM }, // S
	{ KEY('T'),	TERM }, // T
	{ KEY('U'),	TERM }, // U
	{ KEY('V'),	TERM }, // V
	{ KEY('W'),	TERM }, // W

	// 05
	{ KEY('X'),	TERM }, // X
	{ KEY('Y'),	TERM }, // Y
	{ KEY('Z'),	TERM }, // Z
	{ KEY(0xdd),TERM }, // [ ok
	{ KEY(0xdc),TERM }, // \ ok
	{ KEY(0xc0),TERM }, // ]
	{ KEY(0xbb),TERM }, // ^ ok
	{ KEY(0xbd),TERM }, // - ok

	// 06
	{ KEY('0'),	TERM }, // 0
	{ KEY('1'),	TERM }, // 1
	{ KEY('2'),	TERM }, // 2
	{ KEY('3'),	TERM }, // 3
	{ KEY('4'),	TERM }, // 4
	{ KEY('5'),	TERM }, // 5
	{ KEY('6'),	TERM }, // 6
	{ KEY('7'),	TERM }, // 7

	// 07
	{ KEY('8'),	TERM }, // 8
	{ KEY('9'),	TERM }, // 9
	{ KEY(0xba),TERM }, // :
	{ KEY(0xde),TERM }, // ;
	{ KEY(0xbc),TERM }, // ,
	{ KEY(0xbe),TERM }, // .
	{ KEY(0xbf),TERM }, // /
	{ KEY(0xe2),TERM }, // _

	// 08
	{ KEYF(VK_HOME, ext),	TERM }, // CLR
	{ KEYF(VK_UP, noarrowtenex),	KEYF(VK_DOWN, pc80key), TERM }, // ↑
	{ KEYF(VK_RIGHT, noarrowtenex),	KEYF(VK_LEFT, pc80key), TERM }, // →
	{ KEY(VK_BACK),	KEYF(VK_INSERT, ext), KEYF(VK_DELETE, ext), TERM }, // BS
	{ KEY(VK_MENU),			TERM }, // GRPH
	{ KEYF(VK_SCROLL, lock),TERM }, // カナ
	{ KEY(VK_SHIFT), KEY(VK_F6), KEY(VK_F7), KEY(VK_F8), KEY(VK_F9), KEY(VK_F10), KEYF(VK_INSERT, ext), KEYF(1, pc80sft) }, // SHIFT
	{ KEY(VK_CONTROL),		TERM }, // CTRL

	// 09
	{ KEY(VK_F11),KEY(VK_PAUSE), TERM }, // STOP
	{ KEY(VK_F1), KEY(VK_F6),	TERM }, // F1
	{ KEY(VK_F2), KEY(VK_F7),	TERM }, // F2
	{ KEY(VK_F3), KEY(VK_F8),	TERM }, // F3
	{ KEY(VK_F4), KEY(VK_F9),	TERM }, // F4
	{ KEY(VK_F5), KEY(VK_F10),	TERM }, // F5
	{ KEY(VK_SPACE), KEY(VK_CONVERT), KEY(VK_NONCONVERT), TERM }, // SPACE
	{ KEY(VK_ESCAPE),	TERM }, // ESC

	// 0a
	{ KEY(VK_TAB),			TERM }, // TAB
	{ KEYF(VK_DOWN, noarrowtenex),	TERM }, // ↓
	{ KEYF(VK_LEFT, noarrowtenex),	TERM }, // ←
	{ KEYF(VK_END, ext), KEY(VK_HELP), TERM }, // HELP
	{ KEY(VK_F12), TERM }, // COPY
	{ KEY(0x6d),			TERM }, // -
	{ KEY(0x6f),			TERM }, // /
	{ KEYF(VK_CAPITAL, lock), TERM }, // CAPS LOCK

	// 0b
	{ KEYF(VK_NEXT, ext),	TERM }, // ROLL DOWN
	{ KEYF(VK_PRIOR, ext),	TERM }, // ROLL UP
	{ TERM, },
	{ TERM, },
	{ TERM, },
	{ TERM, },
	{ TERM, },
	{ TERM, },

	// 0c
	{ KEY(VK_F6), TERM },			// F6
	{ KEY(VK_F7), TERM },			// F7
	{ KEY(VK_F8), TERM },			// F8
	{ KEY(VK_F9), TERM },			// F9
	{ KEY(VK_F10), TERM },			// F10
	{ KEY(VK_BACK), TERM },			// BS
	{ KEYF(VK_INSERT, ext), TERM },	// INS
	{ KEYF(VK_DELETE, ext), TERM },	// DEL

	// 0d
	{ KEY(VK_CONVERT), TERM },		// 変換
	{ KEY(VK_NONCONVERT), KEY(VK_ACCEPT), TERM }, // 決定
	{ TERM },						// PC
	{ KEY(0xf4), TERM },			// 全角
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },

	// 0e
	{ KEYF(VK_RETURN, nex), TERM },		// RET FK
	{ KEYF(VK_RETURN, ext), TERM },		// RET 10
	{ KEY(VK_LSHIFT), TERM },		// SHIFT L
	{ KEY(VK_RSHIFT), TERM },		// SHIFT R
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },

	// 0f
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
};

// ---------------------------------------------------------------------------
//	キー対応表 for 9801 key
//
const WinKeyIF::Key WinKeyIF::KeyTable98[16 * 8][8] =
{
	// 00
	{ KEY(VK_NUMPAD0), TERM, },		// num 0
	{ KEY(VK_NUMPAD1), TERM, },		// num 1
	{ KEY(VK_NUMPAD2), KEYF(VK_DOWN, arrowten), TERM, },		// num 2
	{ KEY(VK_NUMPAD3), TERM, },		// num 3
	{ KEY(VK_NUMPAD4), KEYF(VK_LEFT, arrowten), TERM, },		// num 4
	{ KEY(VK_NUMPAD5), TERM, },		// num 5
	{ KEY(VK_NUMPAD6), KEYF(VK_RIGHT,arrowten), TERM, },		// num 6
	{ KEY(VK_NUMPAD7), TERM, },		// num 7

	// 01
	{ KEY(VK_NUMPAD8), KEYF(VK_UP  , arrowten), TERM, },		// num 8
	{ KEY(VK_NUMPAD9), TERM, },		// num 9
	{ KEY(VK_MULTIPLY), TERM, },	// num *
	{ KEY(VK_ADD),		TERM, },	// num +
	{ KEY(0x92),		TERM, },	// num =
	{ KEY(VK_SEPARATOR), TERM, },	// num ,
	{ KEY(VK_DECIMAL),	TERM, },	// num .
	{ KEY(VK_RETURN),	TERM, },	// RET

	// 02
	{ KEY(0xc0),TERM },	// @
	{ KEY('A'),	TERM }, // A
	{ KEY('B'),	TERM }, // B
	{ KEY('C'),	TERM }, // C
	{ KEY('D'),	TERM }, // D
	{ KEY('E'),	TERM }, // E
	{ KEY('F'),	TERM }, // F
	{ KEY('G'),	TERM }, // G

	// 03
	{ KEY('H'),	TERM }, // H
	{ KEY('I'),	TERM }, // I
	{ KEY('J'),	TERM }, // J
	{ KEY('K'),	TERM }, // K
	{ KEY('L'),	TERM }, // L
	{ KEY('M'),	TERM }, // M
	{ KEY('N'),	TERM }, // N
	{ KEY('O'),	TERM }, // O

	// 04
	{ KEY('P'),	TERM }, // P
	{ KEY('Q'),	TERM }, // Q
	{ KEY('R'),	TERM }, // R
	{ KEY('S'),	TERM }, // S
	{ KEY('T'),	TERM }, // T
	{ KEY('U'),	TERM }, // U
	{ KEY('V'),	TERM }, // V
	{ KEY('W'),	TERM }, // W

	// 05
	{ KEY('X'),	TERM }, // X
	{ KEY('Y'),	TERM }, // Y
	{ KEY('Z'),	TERM }, // Z
	{ KEY(0xdb),TERM }, // [
	{ KEY(0xdc),TERM }, // \ 
	{ KEY(0xdd),TERM }, // ]
	{ KEY(0xde),TERM }, // ^
	{ KEY(0xbd),TERM }, // -

	// 06
	{ KEY('0'),	TERM }, // 0
	{ KEY('1'),	TERM }, // 1
	{ KEY('2'),	TERM }, // 2
	{ KEY('3'),	TERM }, // 3
	{ KEY('4'),	TERM }, // 4
	{ KEY('5'),	TERM }, // 5
	{ KEY('6'),	TERM }, // 6
	{ KEY('7'),	TERM }, // 7

	// 07
	{ KEY('8'),	TERM }, // 8
	{ KEY('9'),	TERM }, // 9
	{ KEY(0xba),TERM }, // :
	{ KEY(0xbb),TERM }, // ;
	{ KEY(0xbc),TERM }, // ,
	{ KEY(0xbe),TERM }, // .
	{ KEY(0xbf),TERM }, // /
	{ KEY(0xdf),TERM }, // _

	// 08
	{ KEY(VK_HOME),		TERM }, // CLR
	{ KEYF(VK_UP, noarrowten),		TERM }, // ↑
	{ KEYF(VK_RIGHT, noarrowten),	TERM }, // →
	{ KEY(VK_BACK),		TERM }, // BS
	{ KEY(VK_MENU),		TERM }, // GRPH
	{ KEYF(0x15, lock),	TERM }, // カナ
	{ KEY(VK_SHIFT), KEY(VK_F6), KEY(VK_F7), KEY(VK_F8), KEY(VK_F9), KEY(VK_F10), KEYF(0, pc80sft) }, // SHIFT
	{ KEY(VK_CONTROL),		TERM }, // CTRL

	// 09
	{ KEY(VK_PAUSE),KEY(VK_F11),KEY(VK_F15),TERM }, // STOP
	{ KEY(VK_F1),	KEY(VK_F6),	TERM }, // F1
	{ KEY(VK_F2),	KEY(VK_F7),	TERM }, // F2
	{ KEY(VK_F3),	KEY(VK_F8),	TERM }, // F3
	{ KEY(VK_F4),	KEY(VK_F9),	TERM }, // F4
	{ KEY(VK_F5),	KEY(VK_F10),TERM }, // F5
	{ KEY(VK_SPACE), KEY(0x1d), KEY(0x19), TERM }, // SPACE
	{ KEY(VK_ESCAPE),	TERM }, // ESC

	// 0a
	{ KEY(VK_TAB),		TERM }, // TAB
	{ KEYF(VK_DOWN, noarrowten),		TERM }, // ↓
	{ KEYF(VK_LEFT, noarrowten),		TERM }, // ←
	{ KEY(VK_END),		TERM }, // HELP
	{ KEY(VK_F14),	KEY(VK_F12),	TERM }, // COPY
	{ KEY(0x6d),		TERM }, // -
	{ KEY(0x6f),		TERM }, // /
	{ KEYF(VK_CAPITAL, lock), TERM }, // CAPS LOCK

	// 0b
	{ KEY(VK_NEXT),		TERM }, // ROLL DOWN
	{ KEY(VK_PRIOR),	TERM }, // ROLL UP
	{ TERM, },
	{ TERM, },
	{ TERM, },
	{ TERM, },
	{ TERM, },
	{ TERM, },

	// 0c
	{ KEY(VK_F1), TERM }, // F1
	{ KEY(VK_F2), TERM }, // F2
	{ KEY(VK_F3), TERM }, // F3
	{ KEY(VK_F4), TERM }, // F4
	{ KEY(VK_F5), TERM }, // F5
	{ KEY(VK_BACK), TERM },	// BS
	{ KEY(VK_INSERT), TERM }, // INS
	{ KEY(VK_DELETE), TERM }, // DEL

	// 0d
	{ KEY(VK_F6),	TERM }, // F6
	{ KEY(VK_F7),	TERM }, // F7
	{ KEY(VK_F8),	TERM }, // F8
	{ KEY(VK_F9),	TERM }, // F9
	{ KEY(VK_F10),	TERM }, // F10
	{ KEY(0x1d),	TERM }, // 変換
	{ KEY(0x19),	TERM }, // 決定
	{ KEY(VK_SPACE), TERM }, // SPACE

	// 0e
	{ KEY(VK_RETURN), TERM }, // RET FK
	{ KEY(VK_RETURN), TERM }, // RET 10
	{ KEY(VK_LSHIFT), TERM }, // SHIFT L
	{ KEY(VK_RSHIFT), TERM }, // SHIFT R
	{ TERM }, // PC
	{ TERM }, // 全角
	{ TERM },
	{ TERM },

	// 0f
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
	{ TERM },
};

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor WinKeyIF::descriptor = { indef, outdef };

const Device::OutFuncPtr WinKeyIF::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &Reset),
	STATIC_CAST(Device::OutFuncPtr, &VSync),
};

const Device::InFuncPtr WinKeyIF::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, &In),
};
