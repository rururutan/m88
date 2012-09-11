// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  画面制御とグラフィックス画面合成
// ---------------------------------------------------------------------------
//	$Id: screen.cpp,v 1.26 2003/09/28 14:35:35 cisc Exp $

#include "headers.h"
#include "pc88/screen.h"
#include "pc88/memory.h"
#include "pc88/config.h"
#include "pc88/crtc.h"
#include "status.h"

#define LOGNAME	"screen"
#include "diag.h"

using namespace PC8801;

#define GVRAMC_BIT	0xf0
#define GVRAMC_CLR	0xc0
#define GVRAM0_SET	0x10
#define GVRAM0_RES	0x00
#define GVRAM1_SET	0x20
#define GVRAM1_RES	0x00
#define GVRAM2_SET	0x80
#define GVRAM2_RES	0x40

#define GVRAMM_BIT	0x20		// 1110
#define GVRAMM_BITF	0xe0		// 1110
#define GVRAMM_SET	0x20
#define GVRAMM_ODD	0x40
#define GVRAMM_EVEN	0x80

const int16 Screen::RegionTable[64] = 
{
	 640,  -1,    0, 128,  128, 256,    0, 256,
	 256, 384,    0, 384,  128, 384,    0, 384,
	 384, 512,    0, 512,  128, 512,    0, 512,
	 256, 512,    0, 512,  128, 512,    0, 512,
	
	 512, 640,    0, 640,  128, 640,    0, 640,
	 256, 640,    0, 640,  128, 640,    0, 640,
	 384, 640,    0, 640,  128, 640,    0, 640,
	 256, 640,    0, 640,  128, 640,    0, 640,
};

// ---------------------------------------------------------------------------
//	原色パレット
//	RGB
const Draw::Palette Screen::palcolor[8] =
{
	{   0,  0,  0 }, {   0,  0,255 }, { 255,  0,  0 }, { 255,  0,255 },
	{   0,255,  0 }, {   0,255,255 }, { 255,255,  0 }, { 255,255,255 },
};

const uint8 Screen::palextable[2][8] =
{
	{	0,  36,  73, 109, 146, 182, 219, 255 },
	{   0, 255, 255, 255, 255, 255, 255, 255 },
};

// ---------------------------------------------------------------------------
// 構築/消滅
//
Screen::Screen(const ID& id)
: Device(id)
{
	CreateTable();
	line400 = false;
	line320 = false;
}

Screen::~Screen()
{
}

// ---------------------------------------------------------------------------
//	初期化
//
bool Screen::Init(IOBus* _bus, Memory* mem, CRTC* _crtc)
{
	bus = _bus;
	memory = mem;
	crtc = _crtc;

	palettechanged = true;
	modechanged = true;
	fv15k = false;
	texttp = false;
	pex = palextable[0];

	bgpal.red = 0;
	bgpal.green = 0;
	bgpal.blue = 0;
	gmask = 0;
	for (int c=0; c<8; c++)
	{
		pal[c].green = c & 4 ? 255 : 0;
		pal[c].red   = c & 2 ? 255 : 0;
		pal[c].blue  = c & 1 ? 255 : 0;
	}
	return true;
}

void IOCALL Screen::Reset(uint,uint)
{
	n80mode = (newmode & 2) != 0;
	palettechanged = true;
	displaygraphics = false;
	textpriority = false;
	grphpriority = false;
	port31 = ~port31;
	Out31(0x31, ~port31);
	modechanged = true;
}

static inline Draw::Palette Avg(Draw::Palette a, Draw::Palette b)
{
	Draw::Palette c;
	c.red   = (a.red   + b.red)   / 2;
	c.green = (a.green + b.green) / 2;
	c.blue  = (a.blue  + b.blue)  / 2;
	return c;
}

// ---------------------------------------------------------------------------
//	パレットを更新
//
bool Screen::UpdatePalette(Draw* draw)
{
	int pmode;
	
	// 53 53 53 V2 32 31 80  CM 53 30 53 53 53 30 dg
	pmode = displaygraphics ? 1 : 0;
	pmode |= (port53 & 1) << 6;
	pmode |= port30 & 0x22;
	pmode |= n80mode ? 0x100 : 0;
	pmode |= line320 ? 0x400 : 0;
	pmode |= (port33 & 0x80) ? 0x800 : 0;
	if (!color)
	{
		pmode |= 0x80 | ((port53 & 14) << 1);
		if (n80mode && (port33 & 0x80) && line320)	// 80SR 320x200x6
			pmode |= (port53 & 0x70) << 8;
	}
	else
	{
		if (n80mode && (port33 & 0x80)) 
			pmode |= (port53 & (line320 ? 6 : 2)) << 1;
	}
//	statusdisplay.Show(10, 0, "SCRN: %.3x", pmode);

	if (pmode != prevpmode || modechanged)
	{
		LOG1("p:%.2x ", pmode);
		palettechanged = true;
		prevpmode = pmode;
	}

	if (palettechanged)
	{
		palettechanged = false;
		// palette parameter is
		//	palette
		//	-textcolor(port30 & 2)
		//	-displaygraphics
		//	port32 & 0x20
		//	-port53 & 1
		//	^port53 & 6 (if not color)

		Draw::Palette xpal[10];
		if (!texttp)
		{
			for (int i=0; i<8; i++)
			{
				xpal[i].red   = pex[pal[i].red];
				xpal[i].green = pex[pal[i].green];
				xpal[i].blue  = pex[pal[i].blue];
			}
		}
		else
		{
			for (int i=0; i<8; i++)
			{
				xpal[i].red   = (pex[pal[i].red]   * 3 + ((i << 7) & 0x100)) / 4;
				xpal[i].green = (pex[pal[i].green] * 3 + ((i << 6) & 0x100)) / 4;
				xpal[i].blue  = (pex[pal[i].blue]  * 3 + ((i << 8) & 0x100)) / 4;
			}
		}
		if (gmask)
		{
			for (int i=0; i<8; i++)
			{
				if (i & ~gmask)
				{
					xpal[i].green = (xpal[i].green / 8) + 0xe0;
					xpal[i].red   = (xpal[i].red   / 8) + 0xe0;
					xpal[i].blue  = (xpal[i].blue  / 8) + 0xe0;
				}
				else
				{
					xpal[i].green = (xpal[i].green / 6) + 0;
					xpal[i].red   = (xpal[i].red   / 6) + 0;
					xpal[i].blue  = (xpal[i].blue  / 6) + 0;
				}
			}
		}

		xpal[8].red = xpal[8].green = xpal[8].blue = 0;
		xpal[9].red   = pex[bgpal.red];
		xpal[9].green = pex[bgpal.green];
		xpal[9].blue  = pex[bgpal.blue];
		
		Draw::Palette palette[0x90];
		Draw::Palette* p = palette;

		int textcolor = port30 & 2 ? 7 : 0;

		if (color)
		{
			LOG2("\ncolor  port53 = %.2x  port32 = %.2x\n", port53, port32);
			//	color mode		GG GG GR GB TE TG TR TB	
			if (port53 & 1)		// hide text plane ?
			{
				for (int gc=0; gc<9; gc++)
				{
					Draw::Palette c = displaygraphics || texttp ? xpal[gc] : xpal[8];

					for (int i=0; i<16; i++)
						*p++ = c;
				}
			}
			else
			{
				for (int gc=0; gc<9; gc++)
				{
					Draw::Palette c = displaygraphics || texttp ? xpal[gc] : xpal[8];
					
					for (int i=0; i<8; i++)
						*p++ = c;
					if (textpriority && gc > 0)
					{
						for (int tc=0; tc<8; tc++)
							*p++ = c;
					}
					else if (texttp)
					{
						for (int tc=0; tc<8; tc++)
							*p++ = Avg(c, palcolor[tc]);
					}
					else
					{
						*p++ = palcolor[0]; 
						for (int tc=1; tc<8; tc++)
							*p++ = palcolor[tc | textcolor];
					}
				}

				if (fv15k)
				{
					for (int i=0x80; i<0x90; i++)
						palette[i] = palcolor[0];
				}
			}
		}
		else
		{
			//	b/w mode	0  1  G  RE TE   TG TB TR

			const Draw::Palette* tpal = port32 & 0x20 ? xpal : palcolor;
			
			LOG3("\nb/w  port53 = %.2x  port32 = %.2x  port30 = %.2x\n", port53, port32, port30);
			if (port53 & 1)		// hidetext
			{
				int m = texttp || displaygraphics ? ~0 : ~1;
				for (int gc=0; gc<4; gc++)
				{
					int x = gc & m;
					if (((~x>>1) & x & 1))
					{
						for (int i=0; i<32; i++)
							*p++ = tpal[(i&7) | textcolor];
					}
					else
					{
						for (int i=0; i<32; i++)
							*p++ = xpal[9];
					}
				}
			}
			else
			{
				int m = texttp || displaygraphics ? ~0 : ~4;
				for (int gc=0; gc<16; gc++)
				{
					int x = gc & m;
					if (((((~x>>3) & (x>>2)) | x) ^ (x>>1)) & 1)
					{
						if ((x & 8) && fv15k)
							for (int i=0; i<8; i++)
								*p++ = xpal[8];
						else
							for (int i=0; i<8; i++)
								*p++ = tpal[i | textcolor];
					}
					else
					{
						for (int i=0; i<8; i++)
							*p++ = xpal[9];
					}
				}
			}
		}

//		for (int gc=0; gc<0x90; gc++)
//			LOG4("P[%.2x] = %.2x %.2x %.2x\n", gc, palette[gc].green, palette[gc].red, palette[gc].blue);
		draw->SetPalette(0x40, 0x90, palette);
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	画面イメージの更新
//	arg:	region		更新領域
//
void Screen::UpdateScreen(uint8* image, int bpl, Draw::Region& region, bool refresh)
{
	// 53 53 53 GR TX  80 V2 32 CL  53 53 53 L4 (b4～b6の配置は変えないこと)
	int gmode = line400 ? 1 : 0;
	gmode |= color ? 0x10 : (port53 & 0x0e);
	gmode |= line320 ? 0x20 : 0;
	gmode |= (port33 & 0x80) ? 0x40 : 0;
	gmode |= n80mode ? 0x80 : 0;
	gmode |= textpriority ? 0x100 : 0;
	gmode |= grphpriority ? 0x200 : 0;
	if (n80mode && (port33 & 0x80))
	{
		if (color)
			gmode |= port53 & (line320 ? 6 : 2);
		else if (line320)
			gmode |= (port53 & 0x70) << 6;
	}

	if (gmode != prevgmode)
	{
		LOG1("g:%.2x ", gmode);
		prevgmode = gmode;
		modechanged = true;
	}
	
	if (modechanged || refresh)
	{
		LOG0("<modechange> ");
		modechanged = false;
		palettechanged = true;
		ClearScreen(image, bpl);
		memset(memory->GetDirtyFlag(), 1, 0x400);
	}
	if (!n80mode)
	{
		if (color)
			UpdateScreen200c(image, bpl, region);
		else
		{
			if (line400)
				UpdateScreen400b(image, bpl, region);
			else
				UpdateScreen200b(image, bpl, region);
		}
	}
	else
	{
		switch((gmode>>4) & 7) {
		case 0:	UpdateScreen80b(image, bpl, region);  break; //	V1 640x200 B&W
		case 1:	UpdateScreen200c(image, bpl, region); break; //	V1 640x200 COLOR
		case 2:										  break; //	V1 320x200 は常にCOLORなのでB&Wは存在しない
		case 3:	UpdateScreen80c(image, bpl, region);  break; //	V1 320x200 COLOR
		case 4:	UpdateScreen200b(image, bpl, region); break; //	V2 640x200 B&W
		case 5:	UpdateScreen200c(image, bpl, region); break; //	V2 640x200 COLOR
		case 6:	UpdateScreen320b(image, bpl, region); break; //	V2 320x200 B&W
		case 7:	UpdateScreen320c(image, bpl, region); break; //	V2 320x200 COLOR
		}
	}
}


// ---------------------------------------------------------------------------
//	画面更新
//
#define WRITEC0(d, a)	d = (d & ~PACK(GVRAMC_BIT)) \
			| BETable0[(a>>4)&15] | BETable1[(a>>12)&15] | BETable2[(a>>20)&15]

#define WRITEC1(d, a)	d = (d & ~PACK(GVRAMC_BIT)) \
			| BETable0[ a    &15] | BETable1[(a>> 8)&15] | BETable2[(a>>16)&15]

#define WRITEC0F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMC_BIT)) \
			| BETable0[(a>>4)&15] | BETable1[(a>>12)&15] | BETable2[(a>>20)&15]

#define WRITEC1F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMC_BIT)) \
			| BETable0[ a    &15] | BETable1[(a>> 8)&15] | BETable2[(a>>16)&15]

// 640x200, 3 plane color
void Screen::UpdateScreen200c(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += 2 * bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;
		int dm = 0;

		if (!fullline)
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						dm |= 1 << x;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITEC0(d[0], s[0].pack); WRITEC1(d[1], s[0].pack);
							WRITEC0(d[2], s[1].pack); WRITEC1(d[3], s[1].pack);
							WRITEC0(d[4], s[2].pack); WRITEC1(d[5], s[2].pack);
							WRITEC0(d[6], s[3].pack); WRITEC1(d[7], s[3].pack);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		else
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						dm |= 1 << x;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITEC0F(0, s[0].pack); WRITEC1F(1, s[0].pack);
							WRITEC0F(2, s[1].pack); WRITEC1F(3, s[1].pack);
							WRITEC0F(4, s[2].pack); WRITEC1F(5, s[2].pack);
							WRITEC0F(6, s[3].pack); WRITEC1F(7, s[3].pack);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		region.Update(RegionTable[dm * 2], 2 * begin, RegionTable[dm * 2 + 1], 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	画面更新 (200 lines  b/w)
//
#define WRITEB0(d, a)	d = (d & ~PACK(GVRAMM_BIT)) \
			| BETable1[((a>>4) | (a>>12) | (a>>20)) & 15]

#define WRITEB1(d, a)	d = (d & ~PACK(GVRAMM_BIT)) \
			| BETable1[(a    | (a>> 8) | (a>>16)) & 15]

#define WRITEB0F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMM_BIT)) \
			| BETable1[((a>>4) | (a>>12) | (a>>20)) & 15]

#define WRITEB1F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMM_BIT)) \
			| BETable1[(a    | (a>> 8) | (a>>16)) & 15]

// 640x200, b/w
void Screen::UpdateScreen200b(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += 2 * bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;

		Memory::quadbyte mask;
		mask.byte[0] = port53 & 2 ? 0x00 : 0xff;
		mask.byte[1] = port53 & 4 ? 0x00 : 0xff;
		mask.byte[2] = port53 & 8 ? 0x00 : 0xff;
		mask.byte[3] = 0;

		int dm = 0;
		if (!fullline)
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						dm |= 1 << x;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							uint32 x;
							x = s[0].pack & mask.pack; WRITEB0(d[0], x); WRITEB1(d[1], x);
							x = s[1].pack & mask.pack; WRITEB0(d[2], x); WRITEB1(d[3], x);
							x = s[2].pack & mask.pack; WRITEB0(d[4], x); WRITEB1(d[5], x);
							x = s[3].pack & mask.pack; WRITEB0(d[6], x); WRITEB1(d[7], x);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		else
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						dm |= 1 << x;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							uint32 x;
							x = s[0].pack & mask.pack; WRITEB0F(0, x); WRITEB1F(1, x);
							x = s[1].pack & mask.pack; WRITEB0F(2, x); WRITEB1F(3, x);
							x = s[2].pack & mask.pack; WRITEB0F(4, x); WRITEB1F(5, x);
							x = s[3].pack & mask.pack; WRITEB0F(6, x); WRITEB1F(7, x);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		region.Update(RegionTable[dm * 2], 2 * begin, RegionTable[dm * 2 + 1], 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	画面更新 (400 lines  b/w)
//
#define WRITE400B(d, a)	(d)[0] = ((d)[0] & ~PACK(GVRAMM_BIT)) | BETable1[(a >> 4) & 15], \
						(d)[1] = ((d)[1] & ~PACK(GVRAMM_BIT)) | BETable1[(a >> 0) & 15]

void Screen::UpdateScreen400b(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;

		Memory::quadbyte mask;
		mask.byte[0] = port53 & 2 ? 0x00 : 0xff;
		mask.byte[1] = port53 & 4 ? 0x00 : 0xff;
		mask.byte[2] = port53 & 8 ? 0x00 : 0xff;
		mask.byte[3] = 0;

		int dm = 0;
		for (; y<200; y++, image += bpl)
		{
			uint8* dest0 = image;
			uint8* dest1 = image + 200*bpl;

			for (int x=0; x<5; x++, dirty++, src += 16, dest0 += 128, dest1 += 128)
			{
				if (*dirty)
				{
					*dirty = 0;
					end = y;
					dm |= 1 << x;
					
					Memory::quadbyte* s = src;
					packed* d0 = (packed*) dest0;
					packed* d1 = (packed*) dest1;
					for (int j=0; j<4; j++)
					{
						WRITE400B(d0  , s[0].byte[0]); WRITE400B(d1  , s[0].byte[1]);
						WRITE400B(d0+2, s[1].byte[0]); WRITE400B(d1+2, s[1].byte[1]);
						WRITE400B(d0+4, s[2].byte[0]); WRITE400B(d1+4, s[2].byte[1]);
						WRITE400B(d0+6, s[3].byte[0]); WRITE400B(d1+6, s[3].byte[1]);
						d0 += 8, d1 += 8, s += 4;
					}
				}
			}
		}
		region.Update(RegionTable[dm * 2], begin, RegionTable[dm * 2 + 1], 200 + end);
	}
}

// ---------------------------------------------------------------------------
//	画面更新
//
#define WRITE80C0(d, a)		d = (d & ~PACK(GVRAMC_BIT)) | E80Table[(a >> 4) &15]

#define WRITE80C1(d, a)		d = (d & ~PACK(GVRAMC_BIT)) | E80Table[a & 15]

#define WRITE80C0F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMC_BIT)) \
							 | E80Table[(a >> 4) & 15]
#define WRITE80C1F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMC_BIT)) \
							 | E80Table[a & 15]

// 320x200, color?
void Screen::UpdateScreen80c(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += 2 * bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;
		int dm = 0;
		
		if (!fullline)
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						dm |= 1 << x;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITE80C0(d[0], s[0].byte[0]); WRITE80C1(d[1], s[0].byte[0]);
							WRITE80C0(d[2], s[1].byte[0]); WRITE80C1(d[3], s[1].byte[0]);
							WRITE80C0(d[4], s[2].byte[0]); WRITE80C1(d[5], s[2].byte[0]);
							WRITE80C0(d[6], s[3].byte[0]); WRITE80C1(d[7], s[3].byte[0]);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		else
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						dm |= 1 << x;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITE80C0F(0, s[0].byte[0]); WRITE80C1F(1, s[0].byte[0]);
							WRITE80C0F(2, s[1].byte[0]); WRITE80C1F(3, s[1].byte[0]);
							WRITE80C0F(4, s[2].byte[0]); WRITE80C1F(5, s[2].byte[0]);
							WRITE80C0F(6, s[3].byte[0]); WRITE80C1F(7, s[3].byte[0]);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		region.Update(RegionTable[dm * 2], 2 * begin, RegionTable[dm * 2 + 1], 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	画面更新 (200 lines  b/w)
//
#define WRITE80B0(d, a)	d = (d & ~PACK(GVRAMM_BIT)) | BETable1[(a>>4) & 15]

#define WRITE80B1(d, a)	d = (d & ~PACK(GVRAMM_BIT)) | BETable1[(a   ) & 15]

#define WRITE80B0F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMM_BIT)) \
			| BETable1[(a>>4) & 15]

#define WRITE80B1F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMM_BIT)) \
			| BETable1[(a   ) & 15]


void Screen::UpdateScreen80b(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += 2 * bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;

		Memory::quadbyte mask;
		if (!gmask)
		{
			mask.byte[0] = port53 & 2 ? 0x00 : 0xff;
			mask.byte[1] = port53 & 4 ? 0x00 : 0xff;
			mask.byte[2] = port53 & 8 ? 0x00 : 0xff;
		}
		else
		{
			mask.byte[0] = gmask & 1 ? 0x00 : 0xff;
			mask.byte[1] = gmask & 2 ? 0x00 : 0xff;
			mask.byte[2] = gmask & 4 ? 0x00 : 0xff;
		}
		mask.byte[3] = 0;

		int dm = 0;

		if (!fullline)
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						dm |= 1 << x;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITE80B0(d[0], s[0].byte[0]); WRITE80B1(d[1], s[0].byte[0]);
							WRITE80B0(d[2], s[1].byte[0]); WRITE80B1(d[3], s[1].byte[0]);
							WRITE80B0(d[4], s[2].byte[0]); WRITE80B1(d[5], s[2].byte[0]);
							WRITE80B0(d[6], s[3].byte[0]); WRITE80B1(d[7], s[3].byte[0]);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		else
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						dm |= 1 << x;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITE80B0F(0, s[0].byte[0]); WRITE80B1F(1, s[0].byte[0]);
							WRITE80B0F(2, s[1].byte[0]); WRITE80B1F(3, s[1].byte[0]);
							WRITE80B0F(4, s[2].byte[0]); WRITE80B1F(5, s[2].byte[0]);
							WRITE80B0F(6, s[3].byte[0]); WRITE80B1F(7, s[3].byte[0]);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		region.Update(RegionTable[dm * 2], 2 * begin, RegionTable[dm * 2 + 1], 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	画面更新 (320x200x2 color)
//
#define WRITEC320(d)	m = E80SRMask[(bp1 | rp1>>2 | gp1>>4) & 3]; \
						d = (d & ~PACK(GVRAMC_BIT)) \
							| (E80SRTable[(bp1 & 0x03) | (rp1 & 0x0c) | (gp1 & 0x30)] & m) \
							| (E80SRTable[(bp2 & 0x03) | (rp2 & 0x0c) | (gp2 & 0x30)] & ~m);
#define WRITEC320F(o)	m = E80SRMask[(bp1 | rp1>>2 | gp1>>4) & 3]; \
						*((packed*)(((uint8*)(dest+o))+bpl)) = dest[o] = (dest[o] & ~PACK(GVRAMC_BIT)) \
							| (E80SRTable[(bp1 & 0x03) | (rp1 & 0x0c) | (gp1 & 0x30)] & m) \
							| (E80SRTable[(bp2 & 0x03) | (rp2 & 0x0c) | (gp2 & 0x30)] & ~m);
void Screen::UpdateScreen320c(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty1 = memory->GetDirtyFlag();
	uint8* dirty2 = dirty1 + 0x200;
	int y;
	for (y=0; y<500; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty1[y]) || *(packed*)(&dirty2[y]))
			break;
	}
	if (y < 500)
	{
		y /= 5;
		
		int begin = y * 2, end = y * 2 + 1;
		
		image += 4 * bpl * y;
		dirty1 += 5 * y;
		dirty2 += 5 * y;

		Memory::quadbyte* src1;
		Memory::quadbyte* src2;
		uint dspoff;
		if (!grphpriority)
		{
			src1 = memory->GetGVRAM() + y * 80;
			src2 = memory->GetGVRAM() + y * 80 + 0x2000;
			dspoff = port53;
		}
		else
		{
			src1 = memory->GetGVRAM() + y * 80 + 0x2000;
			src2 = memory->GetGVRAM() + y * 80;
			dspoff = ((port53 >> 1) & 2) | ((port53 << 1) & 4);
		}
		int dm = 0;

		uint	bp1, rp1, gp1, bp2, rp2, gp2;
		bp1 = rp1 = gp1 = bp2 = rp2 = gp2 = 0;

		if (!fullline)
		{

			for (; y<100; y++, image += 4*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty1++, dirty2++)
				{
					if (*dirty1 || *dirty2)
					{
						*dirty1 = *dirty2 = 0;
						end = y * 2 + 1;
						static const int tmp[5] = { 0x03,0x0c,0x11,0x06,0x18 };
						dm |= tmp[x];
						
						packed	m;
						for (int j=0; j<16; j++)
						{
							if (!(dspoff & 2))
							{
								bp1 = src1->byte[0];		
								rp1 = src1->byte[1] << 2;	
								gp1 = src1->byte[2] << 4;
							}
							if (!(dspoff & 4))
							{
								bp2 = src2->byte[0];
								rp2 = src2->byte[1] << 2;
								gp2 = src2->byte[2] << 4;
							}

							WRITEC320(dest[3]);
							bp1 >>= 2; rp1 >>= 2; gp1 >>= 2; bp2 >>= 2; rp2 >>= 2; gp2 >>= 2;
							WRITEC320(dest[2]);
							bp1 >>= 2; rp1 >>= 2; gp1 >>= 2; bp2 >>= 2; rp2 >>= 2; gp2 >>= 2;
							WRITEC320(dest[1]);
							bp1 >>= 2; rp1 >>= 2; gp1 >>= 2; bp2 >>= 2; rp2 >>= 2; gp2 >>= 2;
							WRITEC320(dest[0]);
							if (x == 2 && j == 7) 
								dest = (packed*)(image + 2*bpl);
							else
								dest += 4;
							src1++; src2++;
						}
					}
					else
					{
						src1 += 16; src2 += 16;
						dest = (x == 2 ? (packed*)(image + 2*bpl) + 32 : dest + 64);
					}
				}
			}
		}
		else
		{
			for (; y<100; y++, image += 4*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty1++, dirty2++)
				{
					if (*dirty1 || *dirty2)
					{
						*dirty1 = *dirty2 = 0;
						end = y * 2 + 1;
						static const int tmp[5] = { 0x03,0x0c,0x11,0x06,0x18 };
						dm |= tmp[x];
						
						packed	m;
						for (int j=0; j<16; j++)
						{
							if (!(dspoff & 2))
							{
								bp1 = src1->byte[0];		
								rp1 = src1->byte[1] << 2;	
								gp1 = src1->byte[2] << 4;
							}
							if (!(dspoff & 4))
							{
								bp2 = src2->byte[0];
								rp2 = src2->byte[1] << 2;
								gp2 = src2->byte[2] << 4;
							}

							WRITEC320F(3);
							bp1 >>= 2; rp1 >>= 2; gp1 >>= 2; bp2 >>= 2; rp2 >>= 2; gp2 >>= 2;
							WRITEC320F(2);
							bp1 >>= 2; rp1 >>= 2; gp1 >>= 2; bp2 >>= 2; rp2 >>= 2; gp2 >>= 2;
							WRITEC320F(1);
							bp1 >>= 2; rp1 >>= 2; gp1 >>= 2; bp2 >>= 2; rp2 >>= 2; gp2 >>= 2;
							WRITEC320F(0);
							if (x == 2 && j == 7) 
								dest = (packed*)(image + 2*bpl);
							else
								dest += 4;
							src1++; src2++;
						}
					}
					else
					{
						src1 += 16; src2 += 16;
						dest = (x == 2 ? (packed*)(image + 2*bpl) + 32 : dest + 64);
					}
				}
			}
		}
		region.Update(RegionTable[dm * 2], 2 * begin, RegionTable[dm * 2 + 1], 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	画面更新 (320x200x6 b/w)
//
#define WRITEB320(d, a)	d = (d & ~PACK(GVRAMM_BIT)) \
			| BE80Table[a & 3]

#define WRITEB320F(o, a)	*((packed*)(((uint8*)(dest+o))+bpl)) = dest[o] = (dest[o] & ~PACK(GVRAMM_BIT)) \
			| BE80Table[a & 3]

void Screen::UpdateScreen320b(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty1 = memory->GetDirtyFlag();
	uint8* dirty2 = dirty1 + 0x200;
	int y;
	for (y=0; y<500; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty1[y]) || *(packed*)(&dirty2[y]))
			break;
	}
	if (y < 500)
	{
		y /= 5;
		
		int begin = y * 2, end = y * 2 + 1;
		
		image += 4 * bpl * y;
		dirty1 += 5 * y;
		dirty2 += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;

		Memory::quadbyte mask1;
		Memory::quadbyte mask2;
		mask1.byte[0] = port53 & 2  ? 0x00 : 0xff;
		mask1.byte[1] = port53 & 4  ? 0x00 : 0xff;
		mask1.byte[2] = port53 & 8  ? 0x00 : 0xff;
		mask1.byte[3] = 0;
		mask2.byte[0] = port53 & 16 ? 0x00 : 0xff;
		mask2.byte[1] = port53 & 32 ? 0x00 : 0xff;
		mask2.byte[2] = port53 & 64 ? 0x00 : 0xff;
		mask2.byte[3] = 0;

		int dm = 0;
		if (!fullline)
		{
			for (; y<100; y++, image += 4*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty1++, dirty2++)
				{
					if (*dirty1 || *dirty2)
					{
						*dirty1 = *dirty2 = 0;
						end = y * 2 + 1;
						static const int tmp[5] = { 0x03,0x0c,0x11,0x06,0x18 };
						dm |= tmp[x];
						
						for (int j=0; j<8; j++)
						{
							uint32 s;
							s = (src[0].pack & mask1.pack) | (src[0x2000].pack & mask2.pack);
							s = (s | (s >>8) | (s>>16));
							WRITEB320(dest[3], s); s>>= 2; WRITEB320(dest[2], s); s>>= 2;
							WRITEB320(dest[1], s); s>>= 2; WRITEB320(dest[0], s);
							s = (src[1].pack & mask1.pack) | (src[0x2001].pack & mask2.pack);
							s = (s | (s >>8) | (s>>16));
							WRITEB320(dest[7], s); s>>= 2; WRITEB320(dest[6], s); s>>= 2;
							WRITEB320(dest[5], s); s>>= 2; WRITEB320(dest[4], s);
							if (x == 2 && j == 3) 
								dest = (packed*)(image + 2*bpl);
							else
								dest += 8;
							src += 2;
						}
					}
					else
					{
						src += 16;
						dest = (x == 2 ? (packed*)(image + 2*bpl) + 32 : dest + 64);
					}
				}
			}
		}
		else
		{
			for (; y<100; y++, image += 4*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty1++, dirty2++)
				{
					if (*dirty1 || *dirty2)
					{
						*dirty1 = *dirty2 = 0;
						end = y * 2 + 1;
						static const int tmp[5] = { 0x03,0x0c,0x11,0x06,0x18 };
						dm |= tmp[x];
						
						for (int j=0; j<8; j++)
						{
							uint32 s;
							s = (src[0].pack & mask1.pack) | (src[0x2000].pack & mask2.pack);
							s = (s | (s >>8) | (s>>16));
							WRITEB320F(3, s); s>>= 2; WRITEB320F(2, s); s>>= 2;
							WRITEB320F(1, s); s>>= 2; WRITEB320F(0, s);
							s = (src[1].pack & mask1.pack) | (src[0x2001].pack & mask2.pack);
							s = (s | (s >>8) | (s>>16));
							WRITEB320F(7, s); s>>= 2; WRITEB320F(6, s); s>>= 2;
							WRITEB320F(5, s); s>>= 2; WRITEB320F(4, s);
							if (x == 2 && j == 3) 
								dest = (packed*)(image + 2*bpl);
							else
								dest += 8;
							src += 2;
						}
					}
					else
					{
						src += 16;
						dest = (x == 2 ? (packed*)(image + 2*bpl) + 32 : dest + 64);
					}
				}
			}
		}
		region.Update(RegionTable[dm * 2], 2 * begin, RegionTable[dm * 2 + 1], 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	Out 30
//	b1	CRT モードコントロール
//
void IOCALL Screen::Out30(uint, uint data)
{
//	uint i = port30 ^ data;
	port30 = data;
	crtc->SetTextSize(!(data & 0x01));
}

// ---------------------------------------------------------------------------
//	Out 31
//	b4	color / ~b/w
//	b3	show graphic plane
//	b0	200line / ~400line
//	
void IOCALL Screen::Out31(uint, uint data)
{
	int i = port31 ^ data;
	
	if (!n80mode)
	{
		if (i & 0x19)
		{
			port31 = data;
			displaygraphics = (data & 8) != 0;

			if (i & 0x11)
			{
				color = (data & 0x10) != 0;
				line400 = !(data & 0x01) && !color;
				crtc->SetTextMode(color);
			}
		}
	}
	else
	{
		if (i & 0xfc)
		{
			port31 = data;
			displaygraphics = (data & 8) != 0;

			if (i & 0xf4)
			{
				Pal col;
				col.green = data & 0x80 ? 7 : 0;
				col.red   = data & 0x40 ? 7 : 0;
				col.blue  = data & 0x20 ? 7 : 0;

				if (port33 & 0x80) 
				{
					if (i & 0x1c)
						palettechanged = true;
					line320 = (data & 0x04) != 0;
					color = (data & 0x10) != 0;
					crtc->SetTextMode(color);
					if (!color)
					{
						if (i & 0xe0)
							palettechanged = true;
						bgpal = col;
					}
					if (color)
					{
						uint mask53 = (line320 ? 6 : 2);
						displaygraphics = ((port53 & mask53) != mask53 && (port31 & 8) != 0);
					}
				}
				else
				{
					palettechanged = true;
	
					line320 = (data & 0x10) != 0;
					color = line320 || (data & 0x04) != 0;
					crtc->SetTextMode(color);

					if (!line320)
					{
						pal[0].green = pal[0].red = pal[0].blue = 0;
						for (int j=1; j<8; j++)
							pal[j] = col;
						if (!color) 
							bgpal = col;
					}
					else
					{
						if (data & 0x04)
						{
							pal[0].blue = 7; pal[0].red = 0; pal[0].green = 0;
							pal[1].blue = 7; pal[1].red = 7; pal[1].green = 0;
							pal[2].blue = 7; pal[2].red = 0; pal[2].green = 7;
						}
						else
						{
							pal[0].blue = 0; pal[0].red = 0; pal[0].green = 0;
							pal[1].blue = 0; pal[1].red = 7; pal[1].green = 0;
							pal[2].blue = 0; pal[2].red = 0; pal[2].green = 7;
						}
						pal[3] = col;
						for (int j=0; j<4; j++)
							pal[4+j] = pal[j];
					}
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	Out 32
//	b5	パレットモード
//	
void IOCALL Screen::Out32(uint, uint data)
{
	uint i = port32 ^ data;
	if (i & 0x20)
	{
		port32 = data;
		if (!color)
			palettechanged = true;
	}
}

// ---------------------------------------------------------------------------
//	Out 33
//	b7	0...N/N80,     1...N80V2
//	b3	0...Text>Grph，1...Grph>Text
//	b2	0...Gr1 > Gr2，1...Gr2 > Gr1
//
void IOCALL Screen::Out33(uint, uint data)
{
	if (n80mode) 
	{
		uint i = port33 ^ data;
		if (i & 0x8c)
		{
			port33 = data;
			textpriority = (data & 0x08) != 0;
			grphpriority = (data & 0x04) != 0;
			palettechanged = true;
		}
	}
}

// ---------------------------------------------------------------------------
//	Out 52
//	バックグラウンドカラー(デジタル)の指定
//
void IOCALL Screen::Out52(uint, uint data)
{
	if (!(port32 & 0x20))
	{
		bgpal.blue  = (data & 0x08) ? 255 : 0;
		bgpal.red   = (data & 0x10) ? 255 : 0;
		bgpal.green = (data & 0x20) ? 255 : 0;
		LOG1("bgpalette(d) = %6x\n", bgpal.green*0x10000+bgpal.red*0x100+bgpal.blue);
		if (!color)
			palettechanged = true;
	}
}

// ---------------------------------------------------------------------------
//	Out 53
//	画面重ねあわせの制御
//	
void IOCALL Screen::Out53(uint, uint data)
{
	if (!n80mode) 
	{
		LOG4("show plane(53) : %c%c%c %c\n",
			data & 8 ? '-' : '2', data & 4 ? '-' : '1', 
			data & 2 ? '-' : '0', data & 1 ? '-' : 'T');

		if ((port53 ^ data) & (color ? 0x01 : 0x0f))
		{
			port53 = data;
		}
	}
	else if (port33 & 0x80)
	{
		LOG7("show plane(53) : %c%c%c%c%c%c %c\n",
			data & 64 ? '-' : '5', data & 32 ? '-' : '4', 
			data & 16 ? '-' : '3', data &  8 ? '-' : '2', 
			data &  4 ? '-' : '1', data &  2 ? '-' : '0', 
			data &  1 ? '-' : 'T');
		uint mask;
		if (color)
		{
			if (line320) mask = 0x07;
			else		 mask = 0x03;
		}
		else
		{
			if (line320) mask = 0x7f;
			else		 mask = 0x0f;
		}

		if ((port53 ^ data) & mask)
		{
			modechanged = true;
			if (color)
			{
				uint mask53 = (line320 ? 6 : 2);
				displaygraphics = ((data & mask53) != mask53 && (port31 & 8) != 0);
			}
		}
		port53 = data;	//	画面モードが変更される可能性に備え，値は常に全ビット保存
	}
}

// ---------------------------------------------------------------------------
//	Out 54
//	set palette #0 / BG Color	
//
void IOCALL Screen::Out54(uint, uint data)
{ 
	if (port32 & 0x20)		// is analog palette mode ? 
	{
		Pal& p = data & 0x80 ? bgpal : pal[0];

		if (data & 0x40)
			p.green = data & 7;
		else
			p.blue = data & 7, p.red = (data >> 3) & 7;
		
		LOG2("palette(a) %c = %3x\n", 
			data & 0x80 ? 'b' : '0', pal[0].green*0x100+pal[0].red*0x10+pal[0].blue);
	}
	else
	{
		pal[0].green = data & 4 ? 7 : 0;
		pal[0].red   = data & 2 ? 7 : 0;
		pal[0].blue  = data & 1 ? 7 : 0;
		LOG1("palette(d) 0 = %.3x\n", pal[0].green*0x100+pal[0].red*0x10+pal[0].blue);
	}
	palettechanged = true;
}

// ---------------------------------------------------------------------------
//	Out 55 - 5b
//	Set palette #1 to #7
//	
void IOCALL Screen::Out55to5b(uint port, uint data)
{
	Pal& p = pal[port - 0x54];
	
	if (!n80mode && (port32 & 0x20))		// is analog palette mode?
	{
		if (data & 0x40)
			p.green = data & 7;
		else
			p.blue = data & 7, p.red = (data >> 3) & 7;
	}
	else
	{
		p.green = data & 4 ? 7 : 0;
		p.red   = data & 2 ? 7 : 0;
		p.blue  = data & 1 ? 7 : 0;
	}
	
	LOG2("palette    %d = %.3x\n", port-0x54, p.green*0x100+p.red*0x10+p.blue);
	palettechanged = true;
}


// ---------------------------------------------------------------------------
//	画面消去
//
void Screen::ClearScreen(uint8* image, int bpl)
{
	// COLOR
	
	if (color)
	{
		for (int y=0; y<400; y++, image+=bpl)
		{
			packed* ptr = (packed*) image;

			for (int v=0; v<640/sizeof(packed)/4; v++, ptr+=4)
			{
				ptr[0] = (ptr[0] & ~PACK(GVRAMC_BIT)) | PACK(GVRAMC_CLR);
				ptr[1] = (ptr[1] & ~PACK(GVRAMC_BIT)) | PACK(GVRAMC_CLR);
				ptr[2] = (ptr[2] & ~PACK(GVRAMC_BIT)) | PACK(GVRAMC_CLR);
				ptr[3] = (ptr[3] & ~PACK(GVRAMC_BIT)) | PACK(GVRAMC_CLR);
			}
		}
	}
	else
	{
		bool maskeven = !line400 || n80mode;
		int d = maskeven ? 2 * bpl : bpl;
		
		for (int y=(maskeven ? 200 : 400); y>0; y--, image+=d)
		{
			int v;
			packed* ptr = (packed*) image;

			for (v=0; v<640/sizeof(packed)/4; v++, ptr+=4)
			{
				ptr[0] = (ptr[0] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_ODD);
				ptr[1] = (ptr[1] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_ODD);
				ptr[2] = (ptr[2] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_ODD);
				ptr[3] = (ptr[3] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_ODD);
			}
			
			if (maskeven)
			{
				ptr = (packed*)(image+bpl);
				for (v=0; v<640/sizeof(packed)/4; v++, ptr+=4)
				{
					ptr[0] = (ptr[0] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_EVEN);
					ptr[1] = (ptr[1] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_EVEN);
					ptr[2] = (ptr[2] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_EVEN);
					ptr[3] = (ptr[3] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_EVEN);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	設定更新
//
void Screen::ApplyConfig(const Config* config)
{
	fv15k = config->IsFV15k();
	pex = palextable[(config->flags & Config::digitalpalette) ? 1 : 0];
	texttp = (config->flags & Config::specialpalette) != 0;
	bool flp = fullline;
	fullline = (config->flags & Config::fullline) != 0;
	if (fullline != flp)
		modechanged = true;
	palettechanged = true;
	newmode = config->basicmode;
	gmask = (config->flag2 / Config::mask0) & 7;
}

// ---------------------------------------------------------------------------
//	Table 作成
//
packed Screen::BETable0[1 << sizeof(packed)] = { -1 };
packed Screen::BETable1[1 << sizeof(packed)];
packed Screen::BETable2[1 << sizeof(packed)];
packed Screen::E80Table[1 << sizeof(packed)];
packed Screen::E80SRTable[64];
packed Screen::E80SRMask[4];
packed Screen::BE80Table[4];

#ifdef ENDIAN_IS_BIG
	#define CHKBIT(i, j)	((1 << (sizeof(packed)-j)) & i)
	#define BIT80SR			0
#else
	#define CHKBIT(i, j)	((1 << j) & i)
	#define	BIT80SR			1
#endif

void Screen::CreateTable()
{
	if (BETable0[0] == -1)
	{
		int i;
		for (i=0; i<(1 << sizeof(packed)); i++)
		{
			int j;
			packed p=0, q=0, r=0;

			for (j=0; j<sizeof(packed); j++)
			{
				bool chkbit = CHKBIT(i,j) != 0;
				p = (p << 8) | (chkbit ? GVRAM0_SET : GVRAM0_RES);
				q = (q << 8) | (chkbit ? GVRAM1_SET : GVRAM1_RES);
				r = (r << 8) | (chkbit ? GVRAM2_SET : GVRAM2_RES);
			}
			BETable0[i] = p;
			BETable1[i] = q;
			BETable2[i] = r;
		}

		for (i=0; i<(1 << sizeof(packed)); i++)
		{
			E80Table[i] = BETable0[(i & 0x05) | ((i & 0x05) << 1)]
						| BETable1[(i & 0x0a) | ((i & 0x0a) >> 1)]
						| PACK(GVRAM2_RES);
		}

		for (i=0; i<64; i++)
		{
			packed p;

			p  = (i & 0x01) ? GVRAM0_SET : GVRAM0_RES;
			p |= (i & 0x04) ? GVRAM1_SET : GVRAM1_RES;
			p |= (i & 0x10) ? GVRAM2_SET : GVRAM2_RES;
			E80SRTable[i] = p << (16 * BIT80SR);
			p  = (i & 0x02) ? GVRAM0_SET : GVRAM0_RES;
			p |= (i & 0x08) ? GVRAM1_SET : GVRAM1_RES;
			p |= (i & 0x20) ? GVRAM2_SET : GVRAM2_RES;
			E80SRTable[i] |= p << (16 * (1-BIT80SR));
			E80SRTable[i] |= E80SRTable[i] << 8;
		}

		for (i=0; i<4; i++) 
		{
			E80SRMask[i]  = ((i & 1) ? 0xffff : 0x0000) << (16 * BIT80SR);
			E80SRMask[i] |= ((i & 2) ? 0xffff : 0x0000) << (16 * (1-BIT80SR));
			BE80Table[i]  = ((i & 1) ? GVRAM1_SET : GVRAM1_RES) << (16 * BIT80SR);
			BE80Table[i] |= ((i & 2) ? GVRAM1_SET : GVRAM1_RES) << (16 * (1-BIT80SR));
			BE80Table[i] |= BE80Table[i] << 8;
		}
	}
}

// ---------------------------------------------------------------------------
//	状態保存
//
uint IFCALL Screen::GetStatusSize()
{
	return sizeof(Status);
}

bool IFCALL Screen::SaveStatus(uint8* s)
{
	Status* st = (Status*) s;
	st->rev = ssrev;
	st->p30 = port30;
	st->p31 = port31;
	st->p32 = port32;
	st->p33 = port33;
	st->p53 = port53;
	st->bgpal = bgpal;
	for (int i=0; i<8; i++)
		st->pal[i] = pal[i];
	return true;
}

bool IFCALL Screen::LoadStatus(const uint8* s)
{
	const Status* st = (const Status*) s;
	if (st->rev != ssrev)
		return false;
	Out30(0x30, st->p30);
	Out31(0x31, st->p31);
	Out32(0x32, st->p32);
	Out33(0x33, st->p33);
	Out53(0x53, st->p53);
	bgpal = st->bgpal;
	for (int i=0; i<8; i++)
		pal[i] = st->pal[i];
	modechanged = true;
	return true;
}


// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor Screen::descriptor =
{
	0, Screen::outdef
};

const Device::OutFuncPtr Screen::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &Reset),
	STATIC_CAST(Device::OutFuncPtr, &Out30),
	STATIC_CAST(Device::OutFuncPtr, &Out31),
	STATIC_CAST(Device::OutFuncPtr, &Out32),
	STATIC_CAST(Device::OutFuncPtr, &Out33),
	STATIC_CAST(Device::OutFuncPtr, &Out52),
	STATIC_CAST(Device::OutFuncPtr, &Out53),
	STATIC_CAST(Device::OutFuncPtr, &Out54),
	STATIC_CAST(Device::OutFuncPtr, &Out55to5b),
};

