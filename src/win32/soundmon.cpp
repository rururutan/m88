// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: soundmon.cpp,v 1.15 2003/06/12 14:03:44 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "soundmon.h"
#include "misc.h"
#include "pc88/opnif.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築/消滅
//
OPNMonitor::OPNMonitor()
{
	soundcontrol = 0;
	read = !(write = 0);
	width = 1024;
}

OPNMonitor::~OPNMonitor()
{
}

bool OPNMonitor::Init(OPNIF* o, ISoundControl* sc)
{
	if (!WinMonitor::Init(MAKEINTRESOURCE(IDD_SOUNDMON)))
		return false;
	
	soundcontrol = sc;
	SetUpdateTimer(50);
	
	opn = o;
	regs = o->GetRegs();
	mask = 0;

	buf[0][0] = 0;
	buf[1][0] = 0;


	dim = 12, dimvector = 1;
	return true;
}

void OPNMonitor::DrawMain(HDC hdc, bool)
{
	dim = Limit(dim + dimvector, 12, 0);

	RECT rect;
	GetClientRect(GetHWnd(), &rect);
	
	HBRUSH hbr = CreateSolidBrush(0x113300);
	hbr = (HBRUSH) SelectObject(hdc, hbr);
	PatBlt(hdc, rect.left, rect.top, rect.right, rect.bottom, PATCOPY);
	DeleteObject(SelectObject(hdc, hbr));

	SetTxCol(0x0f0f0f * (4+dim));
	SetBkMode(hdc, TRANSPARENT);

	if (dim <= 4)
		WinMonitor::DrawMain(hdc, true);

	HPEN hpen = CreatePen(PS_SOLID, 0, 0x000f0f * (16-dim));
	hpen = (HPEN) SelectObject(hdc, hpen);

	MoveToEx(hdc, 0, rect.bottom/2, 0);
//	SetPixel(hdc, 0, rect.bottom/2, 0x000f0f * (16-dim));
	
	int r = Min(rect.right, width);
	int j, x;
	for (j=1; j<width - r; j++)
	{
		int y0 = buf[read][j];
		if (y0 > 0 && buf[read][j+1] <= 0)
			break;
	}
	for (x=1; x<r; x++)
	{
		LineTo(hdc, x, rect.bottom/2 + buf[read][j++] / 256);
//		SetPixel(hdc, x, rect.bottom/2 + buf[read][j++] / 128, 0x000f0f * (16-dim));
	}
	if (buf[!read][0] >= width)
	{
		buf[read][0] = 1;
		read = !(write = read);
	}

	DeleteObject(SelectObject(hdc, hpen));

	if (dim > 4)
		WinMonitor::DrawMain(hdc, true);
}


void IFCALL OPNMonitor::Mix(int32* s, int length)
{
	int c = buf[write][0];

	if (c + length >= width)
	{
		while (c<width)
			buf[write][c++] = (s[0] + s[1]) / 2, s += 2;
		buf[write][0] = width;
		return;
	}
	
	for (; length > 0; length--)
	{
		buf[write][c++] = (s[0] + s[1]) / 2, s += 2;
	}
	buf[write][0] = c;
}


// ---------------------------------------------------------------------------
//	ダイアログ処理
//
BOOL OPNMonitor::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CLOSE:
		opn->SetChannelMask(0);
		break;
		
	case WM_SIZE:
		width = Min(LOWORD(lp) + 128, bufsize);
		SetFont(hdlg, Limit(HIWORD(lp) / 32, 24, 8));
		break;

	case WM_LBUTTONUP:
		dimvector = -dimvector;
		break;

	case WM_KEYDOWN:
		{
			if (wp == VK_RETURN)
				mask = 0;
			else
			{
				if (VK_NUMPAD0 <= wp && wp <= VK_NUMPAD9)
					wp -= VK_NUMPAD0;
				else if ('0' <= wp && wp <= '9')
					wp -= '0';
				else if ('A' <= wp && wp <= 'G')
					wp -= 'A' - 10;
				else
					break;

				if (wp)
					mask ^= 1 << (wp - 1);
				else
					mask = ~mask;
			}
			char text[] = "YM2608 [123456789abcdefg]";
			for (int i=0; i<16; i++)
				if (mask & (1 << i))
					text[8+i] = '-';
			SetWindowText(hdlg, text);
			opn->SetChannelMask(mask);
		}
		break;
	}
	return WinMonitor::DlgProc(hdlg, msg, wp, lp);
}

// ---------------------------------------------------------------------------
//	１バイトの数値を16進記述に変換
//
static inline void ToHex(char** p, uint d)
{
	static const char hex[] = "0123456789abcdef";

	*(*p)++ = hex[d >> 4];
	*(*p)++ = hex[d & 15];
}

// ---------------------------------------------------------------------------
//	FN/BLK を F-number に変換
//
static inline uint ToFnum(uint f)
{
	return (f & 2047) << ((f >> 11) & 7);
}

// ---------------------------------------------------------------------------
//	$bx 系の値を変換
//
static inline uint ToFB(uint f)
{
	return (f & 0xff00) | ((f & 0x38) << 1) | (f & 7);
}

// ---------------------------------------------------------------------------
//	状態を表示
//
void OPNMonitor::UpdateText()
{
#if 1
	int i;
	int a = GetLine() * 0x10;
	for (i=0; i<0x20; i++) {
		if (a < 0x200) {
			SetTxCol(0x0f0f0f * (4+dim));
			Putf("%.3x: ", (a & 0x1ff));

			SetTxCol(0x040f04 * (4+dim));
			for (int x=0; x<16; x++) {
				uint d = regs[a & 0x1ff];
				Putf("%.2x ", d & 0xff);
				a++;
			}

			SetTxCol(0x0f0f0f * (4+dim));
			Puts("\n");
		}
	}
#else
	char buf[128];
	int y;

	//	0123456789abcdef0123456789abcdef
	//	AAA -- BBB -- CCC -- NN 4444 1
	//	AAA BBB CCC NN ------ 333 4444 1
	wsprintf(buf, "%.3x --/%.3x --/%.3x --/%.2x %.4x %x  ", 
			 (regs[0] + regs[1] * 256) & 0xfff, 
			 (regs[2] + regs[3] * 256) & 0xfff, 
			 (regs[4] + regs[5] * 256) & 0xfff,
			 regs[6], regs[11] + regs[12] * 256, regs[13] & 0x0f);
	for (y=0; y<3; y++)
	{
		const char* hex = "0123456789abcdef";
		const char* flg = " |-+";
		buf[7*y+4] = flg[((regs[ 7] >> y) &   1) + ((regs[ 7] >> (y+2)) & 2)];
		buf[7*y+5] = regs[8+y] & 16 ? '*' : hex[regs[8+y] & 0xf];
	}
	//	C1 C2 SSSS EEEE DDDD EE LLLL cc
	wsprintf(buf+32, "%.2x %.2x %.4x %.4x %.4x %.2x %.4x %.2x",
			 regs[0x100], regs[0x101],
			 (regs[0x102] + regs[0x103] * 256) & 0xffff, 
			 (regs[0x104] + regs[0x105] * 256) & 0xffff,
			 (regs[0x109] + regs[0x10a] * 256) & 0xffff,
			 regs[0x10b],
			 (regs[0x10c] + regs[0x10d] * 256) & 0xffff,
			 regs[0x110]);
	Putf("%.32s %.32s\n", buf, buf+32);

	//	00001111222233334444555566667777
	//  -- - --- -- -- -- --            
	{
		char* ptr = buf;
		int i;
		for (i=0; i<3; i++) ToHex(&ptr, regs[0x10 + i]), *ptr++ = ' ';
		ptr[-1] = '/';
		for (i=8; i<14; i++) ToHex(&ptr, regs[0x10 + i]), *ptr++ = ' ';
		Putf("%.2x %x %.3x %.2x %.2x %.2x %.2x             %.26s\n",
			regs[0x21], regs[0x22] & 0x0f, (regs[0x24] + regs[0x25] * 256) & 0x3ff,
			regs[0x26], regs[0x27], regs[0x28], regs[0x29],
			buf);
	}
	for (y=3; y<10; y++)
	{
		char* ptr = buf;
		for (int z=0; z<2; z++)
		{
			for (int c=0; c<3; c++)
			{
				for (int s=0; s<4; s++)
				{
					static const int sct[4] = { 0*4, 2*4, 1*4, 3*4 };
					ToHex(&ptr, regs[z*0x100 + y*0x10 + sct[s] + c]);
				}
				*ptr++ = ' ';
			}
		}
		*ptr=0;
		Putf("%x: %.26s    %x: %.26s\n", y, buf, y, buf+27);
	}
	if (regs[0x27] & 0xc0)
	{
		Putf("a: %.5x    %.5x    %.5x %.5x a: %.5x    %.5x    %.5x\n",
			ToFnum(regs[0x0a4]*0x100+regs[0x0a0]), ToFnum(regs[0x0a5]*0x100+regs[0x0a1]), ToFnum(regs[0x0ad]*0x100+regs[0x0a9]), ToFnum(regs[0x0ae]*0x100+regs[0x0aa]),
			ToFnum(regs[0x1a4]*0x100+regs[0x1a0]), ToFnum(regs[0x1a5]*0x100+regs[0x1a1]), ToFnum(regs[0x1a6]*0x100+regs[0x1a2]));
		Putf("b: %.4x  %.4x  %.4x  %.5x %.5x b: %.4x     %.4x     %.4x\n",
			ToFB  (regs[0x0b4]*0x100+regs[0x0b0]), ToFB  (regs[0x0b5]*0x100+regs[0x0b1]), ToFB(regs[0x0b6]*0x100+regs[0x0b2]),
			ToFnum(regs[0x0ac]*0x100+regs[0x0a8]), ToFnum(regs[0x0a6]*0x100+regs[0x0a2]),
			ToFB  (regs[0x1b4]*0x100+regs[0x1b0]), ToFB  (regs[0x1b5]*0x100+regs[0x1b1]), ToFB(regs[0x1b6]*0x100+regs[0x1b2]));
	}
	else
	{
		Putf("a: %.5x    %.5x    %.5x       a: %.5x    %.5x    %.5x\n",
			ToFnum(regs[0x0a4]*0x100+regs[0x0a0]), ToFnum(regs[0x0a5]*0x100+regs[0x0a1]), ToFnum(regs[0x0a6]*0x100+regs[0x0a2]), 
			ToFnum(regs[0x1a4]*0x100+regs[0x1a0]), ToFnum(regs[0x1a5]*0x100+regs[0x1a1]), ToFnum(regs[0x1a6]*0x100+regs[0x1a2]));
		Putf("b: %.4x     %.4x     %.4x        b: %.4x     %.4x     %.4x\n",
			ToFB(regs[0x0b4]*0x100+regs[0x0b0]), ToFB(regs[0x0b5]*0x100+regs[0x0b1]), ToFB(regs[0x0b6]*0x100+regs[0x0b2]),
			ToFB(regs[0x1b4]*0x100+regs[0x1b0]), ToFB(regs[0x1b5]*0x100+regs[0x1b1]), ToFB(regs[0x1b6]*0x100+regs[0x1b2]));
	}
#endif
}

bool OPNMonitor::Connect(ISoundControl* sc)
{
	return true;
}

void OPNMonitor::Start()
{
	if (soundcontrol)
		soundcontrol->Connect(this);
}

void OPNMonitor::Stop()
{
	if (soundcontrol)
		soundcontrol->Disconnect(this);
}