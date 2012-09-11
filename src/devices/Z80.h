// ---------------------------------------------------------------------------
//	Z80 emulator.
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	Z80 のレジスタ定義
// ---------------------------------------------------------------------------
//	$Id: Z80.h,v 1.1.1.1 1999/02/19 09:00:40 cisc Exp $


#ifndef Z80_H
#define Z80_H

#include "types.h"

#define Z80_WORDREG_IN_INT

// ---------------------------------------------------------------------------
//	Z80 のレジスタセット
// ---------------------------------------------------------------------------

struct Z80Reg
{
#ifdef Z80_WORDREG_IN_INT
	#define PAD(p)	uint8 p[sizeof(uint) - sizeof(uint16)]
	typedef uint wordreg;
#else
	#define PAD(p)
	typedef uint16 wordreg;
#endif

	union regs
	{
		struct shorts
		{
			wordreg af;
			wordreg hl, de, bc, ix, iy, sp;
		} w;
		struct words
		{
#ifdef ENDIAN_IS_BIG
			PAD(p1);
			uint8 a, flags;
			PAD(p2);
			uint8 h, l;
			PAD(p3);
			uint8 d, e;
			PAD(p4);
			uint8 b, c;
			PAD(p5);
			uint8 xh, xl;
			PAD(p6);
			uint8 yh, yl;
			PAD(p7);
			uint8 sph, spl;
#else
			uint8 flags, a;
			PAD(p1);
			uint8 l, h;
			PAD(p2);
			uint8 e, d;
			PAD(p3);
			uint8 c, b;
			PAD(p4);
			uint8 xl, xh;
			PAD(p5);
			uint8 yl, yh;
			PAD(p6);
			uint8 spl, sph;
			PAD(p7);
#endif
		} b;
	} r;

	wordreg r_af, r_hl, r_de, r_bc;			/* 裏レジスタ */
	wordreg pc;
	uint8 ireg;
	uint8 rreg, rreg7;						/* R(0-6 bit), R(7th bit) */
	uint8 intmode;							/* 割り込みモード */
	bool iff1, iff2;
};

#undef PAD

#endif /* Z80_H */
