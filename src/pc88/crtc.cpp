// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  CRTC (μPD3301) のエミュレーション
// ---------------------------------------------------------------------------
//	$Id: crtc.cpp,v 1.34 2004/02/05 11:57:49 cisc Exp $

#include "headers.h"
#include "pc88/crtc.h"
#include "pc88/pd8257.h"
#include "pc88/config.h"
#include "pc88/pc88.h"
#include "schedule.h"
#include "draw.h"
#include "misc.h"
#include "file.h"
#include "error.h"
#include "status.h"

//#define LOGNAME "crtc"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	CRTC 部の機能
//	・VSYNC 割り込み管理
//	・画面位置・サイズ計算
//	・テキスト画面生成
//	・CGROM
//
//	カーソルブリンク間隔 16n フレーム
//
//	Status Bit
//		b0		Light Pen
//		b1		E(?)
//		b2		N(?)
//		b3		DMA under run
//		b4		Video Enable
//
//	画面イメージのビット配分
//		GMode	b4 b3 b2 b1 b0
//		カラー	-- TE TG TR TB 
//		白黒	Rv TE TG TR TB
//
//	リバースの方法(XOR)
//		カラー	-- TE -- -- --
//		白黒    Rv -- -- -- --
//
//	24kHz	440 lines(25)
//			448 lines(20)
//	15kHz	256 lines(25)
//			260 lines(20)
//
#define TEXT_BIT	0x0f
#define TEXT_SET	0x08
#define TEXT_RES	0x00
#define COLOR_BIT	0x07

#define TEXT_BITP	PACK(TEXT_BIT)
#define TEXT_SETP	PACK(TEXT_SET)
#define TEXT_RESP	PACK(TEXT_RES)

// ---------------------------------------------------------------------------
// 構築/消滅
//
CRTC::CRTC(const ID& id)
: Device(id)
{
	font = 0;
	fontrom = 0;
	cg80rom = 0;
	vram[0] = 0;
	pcgram = 0;
	pcgadr = 0;
	pcgdat = 0;
	pcgenable = 0;
	kanaenable = false;
	pat_col = 0;
	pat_mask = 0;
	pat_rev = 0;
	status = 0;
	cursor_x = 0;
	cursor_y = 0;
	cursor_type = 0;
	param1 = 0;
}

CRTC::~CRTC()
{
	delete[] font;
	delete[] fontrom;
	delete[] vram[0];
	delete[] pcgram;
	delete[] cg80rom;
}

// ---------------------------------------------------------------------------
//	初期化
//
bool CRTC::Init(IOBus* b, Scheduler* s, PD8257* d, Draw* _draw)
{
	bus = b, scheduler = s, dmac = d, draw = _draw;

	delete[] font;
	delete[] fontrom;
	delete[] vram[0];
	delete[] pcgram;
	
	font = new uint8[0x8000 + 0x10000];
	fontrom = new uint8[0x800];
	vram[0] = new uint8[0x1e00+0x1e00+0x1400];
	pcgram = new uint8[0x400];
	
	if (!font || !fontrom || !vram[0] || !pcgram)
	{
		Error::SetError(Error::OutOfMemory);
		return false;
	}
	if (!LoadFontFile())
	{
		Error::SetError(Error::LoadFontFailed);
		return false;
	}
	CreateTFont();
	CreateGFont();
	
	vram[1] = vram[0] + 0x1e00;
	attrcache = vram[1] + 0x1e00;
	
	bank = 0;
	mode = 0;
	column = 0;
	SetTextMode(true);
	EnablePCG(true);

	sev = 0;
	return true;
}

// ---------------------------------------------------------------------------
//	IO
//
void IOCALL CRTC::Out(uint port, uint data)
{
	Command((port & 1) != 0, data);
}

uint IOCALL CRTC::In(uint)
{
	return Command(false, 0);
}

uint IOCALL CRTC::GetStatus(uint)
{
	return status;
}

// ---------------------------------------------------------------------------
//	Reset
//	
void IOCALL CRTC::Reset(uint, uint)
{
	line200 = (bus->In(0x40) & 2) != 0;
	memcpy(pcgram, fontrom+0x400, 0x400);
	kanamode = 0;
	CreateTFont();
	HotReset();
}

// ---------------------------------------------------------------------------
//	パラメータリセット
//
void CRTC::HotReset()
{
	status = 0;		// 1
	
	cursor_type = cursormode = -1;
//	tvramsize = 0;
	linesize = 0;
//	screenwidth = 640;
	screenheight = 400;

	linetime = line200 ? int(6.258*8) : int(4.028*16);
	height = 25; 
	vretrace = line200 ? 7 : 3;
	mode = clear | resize;

	pcount[0] = 0;
	pcount[1] = 0;

	scheduler->DelEvent(sev);
	StartDisplay();
}

// ---------------------------------------------------------------------------
//	グラフィックモードの変更
//
void CRTC::SetTextMode(bool color)
{
	if (color)
	{
		pat_rev  = PACK(0x08);
		pat_mask = ~PACK(0x0f);
	}
	else
	{
		pat_rev  = PACK(0x10);
		pat_mask = ~PACK(0x1f);
	}
	mode |= refresh;
}

// ---------------------------------------------------------------------------
//	文字サイズの変更
//
void CRTC::SetTextSize(bool wide)
{
	widefont = wide;
	memset(attrcache, secret, 0x1400);
}

// ---------------------------------------------------------------------------
//	コマンド処理
//
uint CRTC::Command(bool a0, uint data)
{
	static const uint modetbl[8] =
	{
		enable | control | attribute,			// transparent b/w
		enable,									// no attribute
		enable | color | control | attribute,	// transparent color
		0,										// invalid
		enable | control | nontransparent,		// non transparent b/w
		enable | nontransparent,				// non transparent b/w
		0,										// invalid
		0,										// invalid
	};

	uint result = 0xff;

	LOG1(a0 ? "\ncmd:%.2x " : "%.2x ", data);
	
	if (a0)
		cmdc = 0, cmdm = data >> 5;
	
	switch (cmdm)
	{
	case 0:				// RESET
		if (cmdc < 6)
			pcount[0] = cmdc+1, param0[cmdc] = data;
		switch (cmdc)
		{
		case 0:	
			status = 0;		// 1
			attr = 7 << 5;
			mode |= clear;
			pcount[1] = 0;
			break;
			
		//	b0-b6	width-2 (char)
		//	b7		???
		case 1:
			width = (data & 0x7f) + 2;
			break;
			
		//	b0-b5	height-1 (char)
		//	b6-b7	カーソル点滅速度 (0:16 - 3:64 frame) 
		case 2:
			blinkrate = 32 * (1 + (data >> 6));
			height = (data & 0x3f) + 1;
			break;
			
		//	b0-b4	文字のライン数
		//	b5-b6	カーソルの種別 (b5:点滅 b6:ボックス/~アンダーライン)
		//	b7		1 行置きモード
		case 3:
			cursormode = (data >> 5) & 3;
			linesperchar = (data & 0x1f) + 1;
			
			linetime = (line200 ? int(6.258*1024) : int(4.028*1024)) * linesperchar / 1024;
			if (data & 0x80)
				mode |= skipline;
			if (line200)
				linesperchar *= 2;

			linecharlimit = Min(linesperchar, 16);
			break;
			
		//	b0-b4	Horizontal Retrace-2 (char)
		//	b5-b7	Vertical Retrace-1 (char)
		case 4:
//			hretrace = (data & 0x1f) + 2;
			vretrace = ((data >> 5) & 7) + 1;
//			linetime = 1667 / (height+vretrace-1);
			break;

		//	b0-b4	１行あたりのアトリビュート数 - 1
		//	b5-b7	テキスト画面モード
		case 5:
			mode &= ~(enable | color | control | attribute | nontransparent);
			mode |= modetbl[(data >> 5) & 7];
			attrperline = mode & attribute ? (data & 0x1f) + 1 : 0;
			if (attrperline + width > 120)
				mode &= ~enable;

//			screenwidth = 640;
			screenheight = Min(400, linesperchar * height);
			LOG5("\nscrn=(640, %d), vrtc = %d, linetime = %d0 us, frametime0 = %d us\n", screenheight, vretrace, linetime, linetime*(height+vretrace));
			mode |= resize;
			break;
		}
		break;
		
		// START DISPLAY
		// b0	invert
	case 1:
		if (cmdc == 0)
		{
			pcount[1] = 1, param1 = data;
			
			linesize = width + attrperline * 2;
			int tvramsize = (mode & skipline ? (height+1) / 2 : height) * linesize;
			
			LOG1("[%.2x]", status);
			mode = (mode & ~inverse) | (data & 1 ? inverse : 0);
			if (mode & enable)
			{
				if (!(status & 0x10))
				{
					status |= 0x10;
					scheduler->DelEvent(sev);
					event = -1;
					sev = scheduler->AddEvent(linetime*vretrace, this, STATIC_CAST(TimeFunc, &CRTC::StartDisplay), 0);
				}
			}
			else
				status &= ~0x10;

			LOG5(" Start Display [%.2x;%.3x;%2d] vrtc %d  tvram size = %.4x ",
				status, mode, width, vretrace, tvramsize);
		}
		break;
			
	case 2:			// SET INTERRUPT MASK
		if (!(data & 1))
		{
			mode |= clear;
			status = 0;
		}
		break;
		
	case 3:			// READ LIGHT PEN
		status &= ~1;
		break;
		
	case 4:			// LOAD CURSOR POSITION
		switch (cmdc)
		{
			// b0	display cursor
		case 0:
			cursor_type = data & 1 ? cursormode : -1;
			break;
		case 1:
			cursor_x = data;
			break;
		case 2:
			cursor_y = data;
			break;
		}
		break;
	
	case 5:			// RESET INTERRUPT
		break;
		
	case 6:			// RESET COUNTERS
		mode |= clear;			// タイミングによっては
		status = 0;				// 消えないこともあるかも？
		break;
		
	default:
		break;
	}
	cmdc++;
	return result;
}

// ---------------------------------------------------------------------------
//	フォントファイル読み込み
//	
bool CRTC::LoadFontFile()
{
	FileIO file;
	
	if (file.Open("FONT80SR.ROM", FileIO::readonly))
	{
		delete[] cg80rom;
		cg80rom = new uint8[0x2000];
		file.Seek(0, FileIO::begin);
		file.Read(cg80rom, 0x2000);
	}
	
	if (file.Open("FONT.ROM", FileIO::readonly))
	{
		file.Seek(0, FileIO::begin);
		file.Read(fontrom, 0x800);
		return true;
	}
	if (file.Open("KANJI1.ROM", FileIO::readonly))
	{
		file.Seek(0x1000, FileIO::begin);
		file.Read(fontrom, 0x800);
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	テキストフォントから表示用フォントイメージを作成する
//	src		フォント ROM
//
void CRTC::CreateTFont()
{
	CreateTFont(fontrom, 0, 0xa0);
	CreateKanaFont();
	CreateTFont(fontrom + 8 * 0xe0, 0xe0, 0x20);
}

void CRTC::CreateKanaFont()
{
	if (kanaenable && cg80rom) {
		CreateTFont(cg80rom + 0x800 * (kanamode>>4), 0x00, 0x100);
	} else {
		CreateTFont(fontrom + 8 * 0xa0, 0xa0, 0x40);
	}
}

void CRTC::CreateTFont(const uint8* src, int idx, int num)
{
	uint8* dest = font + 64 * idx;
	uint8* destw = font + 0x8000 + 128 * idx;

	for (int i=0; i<num*8; i++)
	{
		uint8 d = *src++;
		for (uint j=0; j<8; j++, d*=2)
		{
			uint8 b = d & 0x80 ? TEXT_SET : TEXT_RES;
			*dest++ = b; *destw++ = b; *destw++ = b;
		}
	}
}

void CRTC::ModifyFont(uint off, uint d)
{
	uint8* dest = font + 8 * off;
	uint8* destw = font + 0x8000 + 16 * off;
	
	for (uint j=0; j<8; j++, d*=2)
	{
		uint8 b = d & 0x80 ? TEXT_SET : TEXT_RES;
		*dest++ = b; *destw++ = b; *destw++ = b;
	}
	mode |= refresh;
}


// ---------------------------------------------------------------------------
//	セミグラフィックス用フォントを作成する
//	
void CRTC::CreateGFont()
{
	uint8* dest = font + 0x4000;
	uint8* destw = font + 0x10000;
	const uint8 order[8] = { 0x01, 0x10, 0x02, 0x20, 0x04, 0x40, 0x08, 0x80 };
	
	for (int i=0; i<256; i++)
	{
		for (uint j=0; j<8; j+=2)
		{
			dest [ 0] = dest [ 1] = dest [ 2] = dest [ 3] =
			dest [ 8] = dest [ 9] = dest [10] = dest [11] =
			destw[ 0] = destw[ 1] = destw[ 2] = destw[ 3] =
			destw[ 4] = destw[ 5] = destw[ 6] = destw[ 7] =
			destw[16] = destw[17] = destw[18] = destw[19] =
			destw[20] = destw[21] = destw[22] = destw[23] =
				i & order[j] ? TEXT_SET : TEXT_RES;
			
			dest [ 4] = dest [ 5] = dest [ 6] = dest [ 7] =
			dest [12] = dest [13] = dest [14] = dest [15] =
			destw[ 8] = destw[ 9] = destw[10] = destw[11] =
			destw[12] = destw[13] = destw[14] = destw[15] =
			destw[24] = destw[25] = destw[26] = destw[27] =
			destw[28] = destw[29] = destw[30] = destw[31] =
				i & order[j+1] ? TEXT_SET : TEXT_RES;
			
			dest += 16; destw += 32;
		}
	}
}

// ---------------------------------------------------------------------------
//	画面表示開始のタイミング処理
//
void IOCALL CRTC::StartDisplay(uint)
{
	sev = 0;
	column = 0;
	mode &= ~suppressdisplay;
//	LOG0("DisplayStart\n");
	bus->Out(PC88::vrtc, 0);
	if (++frametime > blinkrate)
		frametime = 0;
	ExpandLine();
}

// ---------------------------------------------------------------------------
//	１行分取得
//
void IOCALL CRTC::ExpandLine(uint)
{
	int e = ExpandLineSub();
	if (e)
	{
		event = e+1;
		sev = scheduler->AddEvent(linetime * e, this, 
							STATIC_CAST(TimeFunc, &CRTC::ExpandLineEnd));
	}
	else
	{
		if (++column < height)
		{
			event = 1;
			sev = scheduler->AddEvent(linetime, this, 
								STATIC_CAST(TimeFunc, &CRTC::ExpandLine));
		}
		else
			ExpandLineEnd();
	}
}


int CRTC::ExpandLineSub()
{
	uint8* dest;
	dest = vram[bank] + linesize * column;
	if (!(mode & skipline) || !(column & 1))
	{
		if (status & 0x10)
		{
			if (linesize > dmac->RequestRead(dmabank, dest, linesize))
			{
				// DMA アンダーラン
				mode = (mode & ~(enable)) | clear;
				status = (status & ~0x10) | 0x08;
				memset(dest, 0, linesize);
				LOG0("DMA underrun\n");
			}
			else
			{
				if (mode & suppressdisplay)
					memset(dest, 0, linesize);

				if (mode & control)
				{
					bool docontrol = false;
#if 0		// XXX: 要検証
					for (int i=1; i<=attrperline; i++)
					{
						if ((dest[linesize-i*2] & 0x7f) == 0x60)
						{
							docontrol = true;
							break;
						}
					}
#else
					docontrol = (dest[linesize-2] & 0x7f) == 0x60;
#endif
					if (docontrol)
					{
						// 特殊制御文字
						int sc = dest[linesize-1];
						if (sc & 1)
						{
							int skip = height - column - 1;
							if (skip)
							{
								memset(dest + linesize, 0, linesize * skip);
								return skip;
							}
						}
						if (sc & 2)
							mode |= suppressdisplay;
					}
				}
			}
		}
		else
			memset(dest, 0, linesize);
	}
	return 0;
}


inline void IOCALL CRTC::ExpandLineEnd(uint)
{
//	LOG0("Vertical Retrace\n");
	bus->Out(PC88::vrtc, 1);
	event = -1;
	sev = scheduler->AddEvent(linetime*vretrace, this, STATIC_CAST(TimeFunc, &CRTC::StartDisplay), 0);
}

// ---------------------------------------------------------------------------
//	画面サイズ変更の必要があれば変更
//
void CRTC::SetSize()
{
}

// ---------------------------------------------------------------------------
//	画面をイメージに展開する
//	region	更新領域
//
void CRTC::UpdateScreen(uint8* image, int _bpl, Draw::Region& region, bool ref)
{
	bpl = _bpl;
	Log("UpdateScreen:");
	if (mode & clear)
	{
		Log(" clear\n");
		mode &= ~(clear | refresh);
		ClearText(image);
		region.Update(0, screenheight);
		return;
	}
	if (mode & resize)
	{
		Log(" resize");
		// 仮想画面自体の大きさを変えてしまうのが理想的だが，
		// 色々面倒なので実際はテキストマスクを貼る
		mode &= ~resize;
//		draw->Resize(screenwidth, screenheight);
		ref = true;
	}
	if ((mode & refresh) || ref)
	{
		Log(" refresh");
		mode &= ~refresh;
		ClearText(image);
	}

//	statusdisplay.Show(10, 0, "CRTC: %.2x %.2x %.2x", status, mode, attr);
	if (status & 0x10)
	{
		static const uint8 ctype[5] =
		{
			0, underline, underline, reverse, reverse
		};

		if ((cursor_type & 1) && ( (frametime <= blinkrate/4) ||  (blinkrate/2 <= frametime && frametime <= 3*blinkrate/4)))
			attr_cursor = 0;
		else
			attr_cursor = ctype[1 + cursor_type];

		attr_blink = frametime < blinkrate / 4 ? secret : 0;
		underlineptr = (linesperchar-1) * bpl;

		Log(" update");

//		LOG4("time: %d  cursor: %d(%d)  blink: %d\n", frametime, attr_cursor, cursor_type, attr_blink);
		ExpandImage(image, region);
	}
	Log("\n");
}

// ---------------------------------------------------------------------------
//	テキスト画面消去
//
void CRTC::ClearText(uint8* dest)
{
	uint y;

//	screenheight = 300;
	for (y=0; y<screenheight; y++)
	{
		packed* d = REINTERPRET_CAST(packed*, dest);
		packed mask = pat_mask;

		for (uint x=640/sizeof(packed)/4; x>0; x--)
		{
			d[0] = (d[0] & mask) | TEXT_RESP;
			d[1] = (d[1] & mask) | TEXT_RESP;
			d[2] = (d[2] & mask) | TEXT_RESP;
			d[3] = (d[3] & mask) | TEXT_RESP;
			d += 4;
		}
		dest += bpl;
	}
	
	packed pat0 = colorpattern[0] | TEXT_SETP;
	for (; y<400; y++)
	{
		packed* d = REINTERPRET_CAST(packed*, dest);
		packed mask = pat_mask;
		
		for (uint x=640/sizeof(packed)/4; x>0; x--)
		{
			d[0] = (d[0] & mask) | pat0;
			d[1] = (d[1] & mask) | pat0;
			d[2] = (d[2] & mask) | pat0;
			d[3] = (d[3] & mask) | pat0;
			d += 4;
		}
		dest += bpl;
	}
	// すべてのテキストをシークレット属性扱いにする
	memset(attrcache, secret, 0x1400);
}

// ---------------------------------------------------------------------------
//	画面展開
//
void CRTC::ExpandImage(uint8* image, Draw::Region& region)
{
	static const packed colorpattern[8] =
	{
		PACK(0), PACK(1), PACK(2), PACK(3), PACK(4), PACK(5), PACK(6), PACK(7)
	};

	uint8 attrflag[128];

	int top		= 100;
	int bottom	= -1;
	
	int linestep = linesperchar * bpl;

	int yy = Min(screenheight / linesperchar, height) - 1;
	
//	LOG1("ExpandImage Bank:%d\n", bank);
//	image += y * linestep;
	uint8* src        = vram[bank     ];// + y * linesize;
	uint8* cache      = vram[bank ^= 1];// + y * linesize;
	uint8* cache_attr = attrcache;// + y * width;
	
	uint left = 999;
	int right = -1;
	
	for (int y=0; y<=yy; y++, image += linestep)
	{
		if (!(mode & skipline) || !(y & 1))
		{
			attr &= ~(overline | underline);
			ExpandAttributes(attrflag, src+width, y);
			
			int rightl = -1;
			if (widefont)
			{
				for (uint x=0; x<width; x+=2)
				{
					uint8 a = attrflag[x];
					if ((src[x] ^ cache[x]) | (a ^ cache_attr[x]))
					{
						pat_col = colorpattern[(a >> 5) & 7];
						cache_attr[x] = a;
						rightl = x+1;
						if (x<left) left = x;
						PutCharW((packed*) &image[8*x], src[x], a);
					}
				}
			}
			else
			{
				for (uint x=0; x<width; x++)
				{
					uint8 a = attrflag[x];
//					LOG1("%.2x ", a);
					if ((src[x] ^ cache[x]) | (a ^ cache_attr[x]))
					{
						pat_col = colorpattern[(a >> 5) & 7];
						cache_attr[x] = a;
						rightl = x;
						if (x<left) left = x;
						PutChar((packed*) &image[8*x], src[x], a);
					}
				}
//				LOG0("\n");
			}
			if (rightl >= 0)
			{	
				if (rightl > right)
					right = rightl;
				if (top == 100)
					top = y;
				bottom = y+1;
			}
		}
		src += linesize; cache += linesize; cache_attr += width;
	}
//	LOG0("\n");
	region.Update(left * 8, linesperchar * top, 
		          (right + 1) * 8, linesperchar * bottom - 1);
//	LOG2("Update: from %3d to %3d\n", region.top, region.bottom);
}

// ---------------------------------------------------------------------------
//	アトリビュート情報を展開
//
void CRTC::ExpandAttributes(uint8* dest, const uint8* src, uint y)
{
	int	i;

	if (attrperline == 0)
	{
		memset(dest, 0xe0, 80);
		return;
	}
	
	// コントロールコード有効時にはアトリビュートが1組減るという
	// 記述がどこかにあったけど、嘘ですか？
	uint nattrs = attrperline; // - (mode & control ? 1 : 0);

	// アトリビュート展開
	//	文献では 2 byte で一組となっているが、実は桁と属性は独立している模様
	//	1 byte 目は属性を反映させる桁(下位 7 bit 有効)
	//	2 byte 目は属性値
	memset(dest, 0, 80);
	for (i = 2 * (nattrs - 1); i >= 0; i -= 2)
		dest[src[i] & 0x7f] = 1;

	src++;
	for (i=0; i<width; i++)
	{
		if (dest[i])
			ChangeAttr(*src), src+=2;
		dest[i] = attr;
	}

	// カーソルの属性を反映
	if (cursor_y == y && cursor_x < width)
		dest[cursor_x] ^= attr_cursor;
}

// ---------------------------------------------------------------------------
//	アトリビュートコードを内部のフラグに変換
//	
void CRTC::ChangeAttr(uint8 code)
{
	if (mode & color)
	{
		if (code & 0x8)
		{
			attr = (attr & 0x0f) | (code & 0xf0);
//			attr ^= mode & inverse;
		}
		else
		{
			attr = (attr & 0xf0) | ((code >> 2) & 0xd) | ((code & 1) << 1);
			attr ^= mode & inverse;
			attr ^= ((code & 2) && !(code & 1)) ? attr_blink : 0;
		}
	}
	else
	{
		attr = 0xe0 | ((code >> 2) & 0x0d) | ((code & 1) << 1) | ((code & 0x80) >> 3);
		attr ^= mode & inverse;
		attr ^= ((code & 2) && !(code & 1)) ? attr_blink : 0;
	}
}

// ---------------------------------------------------------------------------
//	フォントのアドレスを取得
//
inline const uint8* CRTC::GetFont(uint c)
{
	return font + c * 64;
}

// ---------------------------------------------------------------------------
//	フォント(40文字)のアドレスを取得
//
inline const uint8* CRTC::GetFontW(uint c)
{
	return font + 0x8000 + c * 128;
}

// ---------------------------------------------------------------------------
//	テキスト表示
//
inline void CRTC::PutChar(packed* dest, uint8 ch, uint8 attr)
{
	const packed* src = 
		(const packed*) GetFont(((attr << 4) & 0x100) + (attr & secret ? 0 : ch));
	
	if (attr & reverse)
		PutReversed(dest, src), PutLineReversed(dest, attr);
	else
		PutNormal  (dest, src), PutLineNormal  (dest, attr);
}

#define NROW				(bpl/sizeof(packed))
#define DRAW(dest, data)	(dest) = ((dest) & pat_mask) | (data)

// ---------------------------------------------------------------------------
//	普通のテキスト文字
//
void CRTC::PutNormal(packed * dest, const packed * src)
{
	uint h;

	for (h = 0; h < linecharlimit; h+=2)
	{
		packed x = *src++ | pat_col;
		packed y = *src++ | pat_col;
		
		DRAW(dest[     0], x);	DRAW(dest[     1], y);
		DRAW(dest[NROW+0], x);	DRAW(dest[NROW+1], y);
		dest += bpl * 2 / sizeof(packed);
	}
	packed p = pat_col | TEXT_RESP;
	for (; h < linesperchar; h++)
	{
		DRAW(dest[0], p);	DRAW(dest[1], p);
		dest += bpl / sizeof(packed);
	}
}

// ---------------------------------------------------------------------------
//	テキスト反転表示
//
void CRTC::PutReversed(packed * dest, const packed * src)
{
	uint h;

	for (h = 0; h < linecharlimit; h+=2)
	{
		packed x = (*src++ ^ pat_rev) | pat_col;
		packed y = (*src++ ^ pat_rev) | pat_col;
		
		DRAW(dest[     0], x);	DRAW(dest[     1], y);
		DRAW(dest[NROW+0], x);	DRAW(dest[NROW+1], y);
		dest += bpl * 2 / sizeof(packed);
	}

	packed p = pat_col ^ pat_rev; 
	for (; h < linesperchar; h++)
	{
		DRAW(dest[     0], p);	DRAW(dest[     1], p);
		dest += bpl / sizeof(packed);
	}
}

// ---------------------------------------------------------------------------
//	オーバーライン、アンダーライン表示
//
void CRTC::PutLineNormal(packed* dest, uint8 attr)
{
	packed d = pat_col | TEXT_SETP;
	if (attr & overline)	// overline
	{
		DRAW(dest[0], d);	DRAW(dest[1], d);
	}
	if ((attr & underline) && linesperchar > 14)
	{
		dest = (packed*)(((uint8*) dest)+underlineptr);
		DRAW(dest[0], d);	DRAW(dest[1], d);
	}
}

void CRTC::PutLineReversed(packed* dest, uint8 attr)
{
	packed d = (pat_col | TEXT_SETP) ^ pat_rev;
	if (attr & overline)
	{
		DRAW(dest[0], d);	DRAW(dest[1], d);
	}
	if ((attr & underline) && linesperchar > 14)
	{
		dest = (packed*)(((uint8*) dest)+underlineptr);
		DRAW(dest[0], d);	DRAW(dest[1], d);
	}
}

// ---------------------------------------------------------------------------
//	テキスト表示(40 文字モード)
//
inline void CRTC::PutCharW(packed* dest, uint8 ch, uint8 attr)
{
	const packed* src = 
		(const packed*) GetFontW(((attr << 4) & 0x100) + (attr & secret ? 0 : ch));
	
	if (attr & reverse)
		PutReversedW(dest, src), PutLineReversedW(dest, attr);
	else
		PutNormalW  (dest, src), PutLineNormalW  (dest, attr);
}

// ---------------------------------------------------------------------------
//	普通のテキスト文字
//
void CRTC::PutNormalW(packed * dest, const packed * src)
{
	uint h;
	packed x, y;

	for (h = 0; h < linecharlimit; h+=2)
	{
		x = *src++ | pat_col;
		y = *src++ | pat_col;
		DRAW(dest[     0], x);	DRAW(dest[     1], y);
		DRAW(dest[NROW+0], x);	DRAW(dest[NROW+1], y);
		
		x = *src++ | pat_col;
		y = *src++ | pat_col;
		DRAW(dest[     2], x);	DRAW(dest[     3], y);
		DRAW(dest[NROW+2], x);	DRAW(dest[NROW+3], y);
		dest += bpl * 2 / sizeof(packed);
	}
	x = pat_col | TEXT_RESP;
	for (; h < linesperchar; h++)
	{
		DRAW(dest[0], x);	DRAW(dest[1], x);
		DRAW(dest[2], x);	DRAW(dest[3], x);
		dest += bpl / sizeof(packed);
	}
}

// ---------------------------------------------------------------------------
//	テキスト反転表示
//
void CRTC::PutReversedW(packed * dest, const packed * src)
{
	uint h;
	packed x, y;

	for (h = 0; h < linecharlimit; h+=2)
	{
		x = (*src++ ^ pat_rev) | pat_col;
		y = (*src++ ^ pat_rev) | pat_col;
		DRAW(dest[     0], x);	DRAW(dest[     1], y);
		DRAW(dest[NROW+0], x);	DRAW(dest[NROW+1], y);
		
		x = (*src++ ^ pat_rev) | pat_col;
		y = (*src++ ^ pat_rev) | pat_col;
		DRAW(dest[     2], x);	DRAW(dest[     3], y);
		DRAW(dest[NROW+2], x);	DRAW(dest[NROW+3], y);

		dest += bpl * 2 / sizeof(packed);
	}

	x = pat_col ^ pat_rev; 
	for (; h < linesperchar; h++)
	{
		DRAW(dest[     0], x);	DRAW(dest[     1], x);
		DRAW(dest[     2], x);	DRAW(dest[     3], x);
		dest += bpl / sizeof(packed);
	}
}

// ---------------------------------------------------------------------------
//	オーバーライン、アンダーライン表示
//
void CRTC::PutLineNormalW(packed* dest, uint8 attr)
{
	packed d = pat_col | TEXT_SETP;
	if (attr & overline)	// overline
	{
		DRAW(dest[0], d);	DRAW(dest[1], d);
		DRAW(dest[2], d);	DRAW(dest[3], d);
	}
	if ((attr & underline) && linesperchar > 14)
	{
		dest = (packed*)(((uint8*) dest)+underlineptr);
		DRAW(dest[0], d);	DRAW(dest[1], d);
		DRAW(dest[2], d);	DRAW(dest[3], d);
	}
}

void CRTC::PutLineReversedW(packed* dest, uint8 attr)
{
	packed d = (pat_col | TEXT_SETP) ^ pat_rev;
	if (attr & overline)
	{
		DRAW(dest[0], d);	DRAW(dest[1], d);
		DRAW(dest[2], d);	DRAW(dest[3], d);
	}
	if ((attr & underline) && linesperchar > 14)
	{
		dest = (packed*)(((uint8*) dest)+underlineptr);
		DRAW(dest[0], d);	DRAW(dest[1], d);
		DRAW(dest[2], d);	DRAW(dest[3], d);
	}
}

// ---------------------------------------------------------------------------
//	OUT
//
void IOCALL CRTC::PCGOut(uint p, uint d)
{
	switch (p)
	{
	case 0:
		pcgdat = d;
		break;
	case 1:
		pcgadr = (pcgadr & 0xff00) | d;
		break;
	case 2:
		pcgadr = (pcgadr & 0x00ff) | (d << 8);
		break;
	}

	if (pcgadr & 0x1000)
	{
		uint tmp = (pcgadr & 0x2000) ? fontrom[0x400 + (pcgadr & 0x3ff)] : pcgdat;
		LOG2("PCG: %.4x <- %.2x\n", pcgadr, tmp);
		pcgram[pcgadr & 0x3ff] = tmp;
		if (pcgenable)
			ModifyFont(0x400 + (pcgadr & 0x3ff), tmp);
	}
}

// ---------------------------------------------------------------------------
//	OUT
//
void CRTC::EnablePCG(bool enable)
{
	pcgenable = enable;
	if (!pcgenable)
	{
		CreateTFont();
		mode |= refresh;
	}
	else
	{
		for (int i=0; i<0x400; i++)
			ModifyFont(0x400 + i, pcgram[i]);
	}
}

// ---------------------------------------------------------------------------
//	OUT 33H (80SR)
//	bit4 = ひらがな(1)・カタカナ(0)選択
//
void IOCALL CRTC::SetKanaMode(uint, uint data)
{
	if (kanaenable) {
		// ROMに3つフォントが用意されているが1以外は切り替わらない。
		data &= 0x10;
	} else {
		data = 0;
	}

	if (data != kanamode)
	{
		kanamode = data;
		CreateKanaFont();
		mode |= refresh;
	}
}

// ---------------------------------------------------------------------------
//	apply config
//
void CRTC::ApplyConfig(const Config* cfg)
{
	kanaenable = cfg->basicmode == Config::N80V2;
	EnablePCG((cfg->flags & Config::enablepcg) != 0);
}

// ---------------------------------------------------------------------------
//	table
//
const packed CRTC::colorpattern[8] =
{
	PACK(0), PACK(1), PACK(2), PACK(3), PACK(4), PACK(5), PACK(6), PACK(7)
};

// ---------------------------------------------------------------------------
//	状態保存
//
uint IFCALL CRTC::GetStatusSize()
{
	return sizeof(Status);
}

bool IFCALL CRTC::SaveStatus(uint8* s)
{
	LOG0("*** Save Status\n");
	Status* st = (Status*) s;
	
	st->rev = ssrev;
	st->cmdm = cmdm;
	st->cmdc = Max(cmdc, 0xff);
	memcpy(st->pcount, pcount, sizeof(pcount));
	memcpy(st->param0, param0, sizeof(param0));
	st->param1 = param1;
	st->cursor_x = cursor_x;
	st->cursor_y = cursor_y;
	st->cursor_t = cursor_type;
	st->attr = attr;
	st->column = column;
	st->mode = mode;
	st->status = status;
	st->event = event;
	st->color = (pat_rev == PACK(0x08));
	return true;
}

bool IFCALL CRTC::LoadStatus(const uint8* s)
{
	LOG0("*** Load Status\n");
	const Status* st = (const Status*) s;
	if (st->rev < 1 || ssrev < st->rev)
		return false;
	int i;
	for (i=0; i<st->pcount[0]; i++)
		Out(i ? 0 : 1, st->param0[i]);
	if (st->pcount[1])
		Out(1, st->param1);
	cmdm = st->cmdm, cmdc = st->cmdc;
	cursor_x = st->cursor_x;
	cursor_y = st->cursor_y;
	cursor_type = st->cursor_t;
	attr = st->attr;
	column = st->column;
	mode = st->mode;
	status = st->status | clear;
	event = st->event;
	SetTextMode(st->color);
	
	scheduler->DelEvent(sev);
	if (event == 1)
		sev = scheduler->AddEvent(linetime, this, STATIC_CAST(TimeFunc, &CRTC::ExpandLine));
	else if (event > 1)
		sev = scheduler->AddEvent(linetime * (event-1), this, STATIC_CAST(TimeFunc, &CRTC::ExpandLineEnd));
	else if (event == -1 || st->rev == 1)
		sev = scheduler->AddEvent(linetime * vretrace, this, STATIC_CAST(TimeFunc, &CRTC::StartDisplay), 0);
	
	return true;
}

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor CRTC::descriptor = { indef, outdef };

const Device::OutFuncPtr CRTC::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &Reset),
	STATIC_CAST(Device::OutFuncPtr, &Out),
	STATIC_CAST(Device::OutFuncPtr, &PCGOut),
	STATIC_CAST(Device::OutFuncPtr, &SetKanaMode),
};

const Device::InFuncPtr CRTC::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, &In),
	STATIC_CAST(Device::InFuncPtr, &GetStatus),
};
