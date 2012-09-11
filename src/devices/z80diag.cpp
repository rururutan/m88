// ---------------------------------------------------------------------------
//	Z80 Disassembler
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: z80diag.cpp,v 1.6 2001/02/21 11:57:16 cisc Exp $

#include "headers.h"
#include "Z80diag.h"
#include "device_i.h"

#include "diag.h"

// ---------------------------------------------------------------------------
//	ç\íz
//
Z80Diag::Z80Diag()
{
}

// ---------------------------------------------------------------------------
//	èâä˙âª
//
bool Z80Diag::Init(IMemoryAccess* b)
{
	mem = b;
	return true;
}

// ---------------------------------------------------------------------------
//	1ñΩóﬂãtÉAÉZÉìÉuÉãÇ∑ÇÈ
//
uint Z80Diag::Disassemble(uint _pc, char* dest)
{
	pc = _pc;
	xmode = usehl;
	uint data = Read8(pc++);
	Expand(dest, Inst[data]);
	return pc;
}

// ---------------------------------------------------------------------------
//	1ñΩóﬂãtÉAÉZÉìÉuÉãÇ∑ÇÈ
//
uint Z80Diag::DisassembleS(uint _pc, char* dest)
{
	pc = _pc;
	xmode = usehl;
	uint data = Read8(pc++);
	Expand(dest, Inst[data])[-1] = ' ';
	return pc;
}

// ---------------------------------------------------------------------------
//	ìWäJÇ∑ÇÈ
//
char* Z80Diag::Expand(char* dest, const char* src)
{
	const char* text = "HLIXIY";
	const char* text2 = "BCDEHL A";
	char c;
	do
	{
		c = *src++;

		if (c != '@')
			*dest++ = c;
		else
		{
			int i;
			switch (*src++)
			{
			case 'b':
				i = Read8(pc++);
				SetHex(dest, i);
				break;
				
			case 'w':
				i = Read8(pc) + Read8(pc+1)*0x100; pc+=2;
				SetHex(dest, i);
				break;
				
			case 'r':
				i = int8(Read8(pc++));
				SetHex(dest, pc + i);
				break;
				
			case 'x':
				*dest++ = text[xmode+0];
				*dest++ = text[xmode+1];
				break;

			case 'm':
				*dest++ = '(';
				*dest++ = text[xmode+0];
				*dest++ = text[xmode+1];
				if (xmode != usehl)
				{
					i = int8(Read8(pc++));
					if (i)
					{
						if (i > 0)
							*dest++ = '+';
						else
							*dest++ = '-', i = -i;
						SetHex(dest, i);
					}
				}
				*dest++ = ')';
				break;

			case 'h':
				if (xmode != usehl)
					*dest++ = text[xmode + 1];
				*dest++ = 'H';
				break;

			case 'l':
				if (xmode != usehl)
					*dest++ = text[xmode + 1];
				*dest++ = 'L';
				break;

			case 'C':		// CBxx ån
			{
				int y;
				if (xmode != usehl)
					y = int8(Read8(pc++));
				i = Read8(pc++);

				for (const char* s = InstCB[i >> 3]; *s; )
					*dest++ = *s++;

				if ((i & 7) != 6)
				{
					*dest++ = text2[i & 7];
					if (xmode == usehl || (i & 0xc0) == 0x40)
						break;
					*dest++ = '=';
				}

				*dest++ = '(';
				*dest++ = text[xmode+0];
				*dest++ = text[xmode+1];
				if (xmode != usehl)
				{
					if (y)
					{
						if (y > 0)
							*dest++ = '+';
						else
							*dest++ = '-', y = -y;
						SetHex(dest, y);
					}
				}
				*dest++ = ')';
				break;
			}

			case 'D':		// DD ån
				xmode = useix;
				goto ddfd;
			case 'F':		// FD ån
				xmode = useiy;
ddfd:
				i = Read8(pc++);
				if ((i & 0xdf) != 0xdd)
				{
					dest = Expand(dest, Inst[i])-1;
				}
				else
				{
					const char* t3 = "DB   ";
					while (*t3)
						*dest++ = *t3++;
					SetHex(dest, xmode == useix ? 0xdd : 0xfd);
					pc--;
				}
				break;

			case 'E':		// ED ån
				i = Read8(pc++);
				if ((i & 0xc0) == 0x40)
				{
					dest = Expand(dest, InstED1[i & 0x3f])-1;
				}
				else if ((i & 0xe4) == 0xa0)
				{
					dest = Expand(dest, InstED2[((i & 0x18) >> 1) | (i & 3)])-1;
				}
				else
				{
					const char* t4 = "DB   0EDH,";
					while (*t4)
						*dest++ = *t4++;
					SetHex(dest, i);
				}
				break;

			default:
				LOG1("unknown directive: @%c\n", src[-1]);
				break;
			}
		}
	} while (c);
	return dest;
}

void Z80Diag::SetHex(char*& dest, uint n)
{
	const char* table = "0123456789ABCDEF";
	if (n < 10)
	{
		*dest++ = '0'+n;
		return;
	}

	int i, v;
	for (i=12; !(v = n >> i) && i>0; i-=4)
		;
	if (v >= 10)
		*dest++ = '0';
	do
	{
		*dest++ = table[(n >> i) & 0x0f];
	} while (i-=4, i>=0);
	*dest++ = 'H';
}

// ---------------------------------------------------------------------------
//	ÇPñΩóﬂå„ÇÃÉAÉhÉåÉXÇãÅÇﬂÇÈ
//
uint Z80Diag::InstInc(uint ad)
{
	return ad + GetInstSize(ad);
}

// ---------------------------------------------------------------------------
//	ÇPñΩóﬂëOÇÃÉAÉhÉåÉXÇãÅÇﬂÇÈ
//	
uint Z80Diag::InstDec(uint ad)
{
	const int scandepth = 8;

	uint r = InstDecSub(ad, scandepth);
	if (r)
		return ad - r;
	for (int i=1; i<=4; i++)
	{
		if (GetInstSize(ad-i) > i)
			return ad - i;
	}
	return ad-1;
}

uint Z80Diag::InstDecSub(uint ad, int depth)
{
	for (int i=1; i<=4; i++)
	{
		if (GetInstSize(ad-i) == i)
		{
			if (!depth || InstDecSub(ad-i, depth-1))
				return i;
		}
	}
	return 0;
}

int Z80Diag::GetInstSize(uint ad)
{
	uint c;
	int  size;
	c = Read8(ad);
	size = SizeTable[c];
	if (size>0)
		return size;
	if (size == -1)
		return 1;
	if (c == 0xed)
	{
		c = Read8(ad+1);
		return (c & 0xc7) == 0x43 ? 4 : 2;	
	}

	// DD/FD
	c = Read8(ad+1);
	if (c == 0xdd || c == 0xed || c == 0xfd)
		return 1;
	if (c == 0xcb)
		return 4;
	size = SizeTable[c];
	if (size == -1)
		return 3;
	return 1+size;
}

const char* Z80Diag::Inst[0x100] = 
{
	"NOP",			"LD   BC,@w",	"LD   (BC),A",	"INC  BC",		"INC  B",		"DEC  B",		"LD   B,@b",	"RLCA",
	"EX   AF,AF'",	"ADD  @x,BC",	"LD   A,(BC)",	"DEC  BC",		"INC  C",		"DEC  C",		"LD   C,@b", 	"RRCA",
	"DJNZ @r",		"LD   DE,@w",	"LD   (DE),A",	"INC  DE",		"INC  D",		"DEC  D",		"LD   D,@b",	"RLA",
	"JR   @r",		"ADD  @x,DE",	"LD   A,(DE)",  "DEC  DE",		"INC  E",		"DEC  E",		"LD   E,@b", 	"RRA",
	"JR   NZ,@r",	"LD   @x,@w",	"LD   (@w),@x",	"INC  @x",		"INC  @h",		"DEC  @h",		"LD   @h,@b",	"DAA",
	"JR   Z,@r",	"ADD  @x,@x",	"LD   @x,(@w)", "DEC  @x",		"INC  @l",		"DEC  @l",		"LD   @l,@b", 	"CPL",
	"JR   NC,@r",	"LD   SP,@w",	"LD   (@w),A",	"INC  SP",		"INC  @m",		"DEC  @m",		"LD   @m,@b",	"SCF",
	"JR   C,@r",	"ADD  @x,SP",	"LD   A,(@w)",  "DEC  SP",		"INC  A",		"DEC  A",		"LD   A,@b", 	"CCF",
	
	"LD   B,B",		"LD   B,C",		"LD   B,D",		"LD   B,E",		"LD   B,@h",	"LD   B,@l",	"LD   B,@m",	"LD   B,A", 
	"LD   C,B",		"LD   C,C",		"LD   C,D",		"LD   C,E",		"LD   C,@h",	"LD   C,@l",	"LD   C,@m",	"LD   C,A", 
	"LD   D,B",		"LD   D,C",		"LD   D,D",		"LD   D,E", 	"LD   D,@h",	"LD   D,@l",	"LD   D,@m",	"LD   D,A", 
	"LD   E,B",		"LD   E,C",		"LD   E,D",		"LD   E,E",		"LD   E,@h",	"LD   E,@l",	"LD   E,@m",	"LD   E,A", 
	"LD   @h,B",	"LD   @h,C",	"LD   @h,D",	"LD   @h,E",	"LD   @h,@h",	"LD   @h,@l",	"LD   H,@m",	"LD   @h,A",
	"LD   @l,B",	"LD   @l,C",	"LD   @l,D",	"LD   @l,E",	"LD   @l,@h",	"LD   @l,@l",	"LD   L,@m",	"LD   @l,A",
	"LD   @m,B",	"LD   @m,C",	"LD   @m,D",	"LD   @m,E",	"LD   @m,H",	"LD   @m,L",	"HALT",			"LD   @m,A",
	"LD   A,B",		"LD   A,C",		"LD   A,D",		"LD   A,E", 	"LD   A,@h",	"LD   A,@l",	"LD   A,@m",	"LD   A,A", 
	
	"ADD  A,B",		"ADD  A,C",		"ADD  A,D",		"ADD  A,E",		"ADD  A,@h",	"ADD  A,@l",	"ADD  A,@m",	"ADD  A,A",
	"ADC  A,B",		"ADC  A,C",		"ADC  A,D",		"ADC  A,E",		"ADC  A,@h",	"ADC  A,@l",	"ADC  A,@m",	"ADC  A,A",
	"SUB  B",		"SUB  C",		"SUB  D",		"SUB  E",		"SUB  @h",		"SUB  @l",		"SUB  @m",		"SUB  A",
	"SBC  A,B",		"SBC  A,C",		"SBC  A,D",		"SBC  A,E",		"SBC  A,@h",	"SBC  A,@l",	"SBC  A,@m",	"SBC  A,A",
	"AND  B",		"AND  C",		"AND  D",		"AND  E",		"AND  @h",		"AND  @l",		"AND  @m",		"AND  A",
	"XOR  B",		"XOR  C",		"XOR  D",		"XOR  E",		"XOR  @h",		"XOR  @l",		"XOR  @m",		"XOR  A",
	"OR   B",		"OR   C",		"OR   D",		"OR   E",		"OR   @h",		"OR   @l",		"OR   @m",		"OR   A",
	"CP   B",		"CP   C",		"CP   D",		"CP   E",		"CP   @h",		"CP   @l",		"CP   @m",		"CP   A",
	
	"RET  NZ",		"POP  BC",		"JP   NZ,@w",	"JP   @w",		"CALL NZ,@w",	"PUSH BC",		"ADD  A,@b",	"RST  0",
	"RET  Z",		"RET",			"JP   Z,@w",	"@C",			"CALL Z,@w",	"CALL @w",		"ADC  A,@b",	"RST  8",
	"RET  NC",		"POP  DE",		"JP   NC,@w",	"OUT  (@b),A",	"CALL NC,@w",	"PUSH DE",		"SUB  @b",		"RST  10H",
	"RET  C",		"EXX",			"JP   C,@w",	"IN   A,(@b)",	"CALL C,@w",	"@D",			"SBC  A,@b",	"RST  18H",
	"RET  PO",		"POP  @x",		"JP   PO,@w",	"EX   (SP),@x",	"CALL PO,@w",	"PUSH @x",		"AND  @b",		"RST  20H",
	"RET  PE",		"JP   (@x)",	"JP   PE,@w",	"EX   DE,HL",	"CALL PE,@w",	"@E",			"XOR  @b",		"RST  28H",
	"RET  P",		"POP  AF",		"JP   P,@w",	"DI",			"CALL P,@w",	"PUSH AF",		"OR   @b",		"RST  30H",
	"RET  M",		"LD   SP,@x",	"JP   M,@w",	"EI",			"CALL M,@w",	"@F",			"CP   @b",		"RST  38H",
};

const char* Z80Diag::InstED1[0x40] =
{
	"IN   B,(C)",   "OUT  (C),B",   "SBC  HL,BC",   "LD   (@w),BC",	"NEG",			"RETN",			"IM   0",       "LD   I,A",
	"IN   C,(C)",   "OUT  (C),C",   "ADC  HL,BC",   "LD   BC,(@w)",	"(NEG)",		"RETI",			"(IM  0)",      "LD   A,I",
	"IN   D,(C)",   "OUT  (C),D",   "SBC  HL,DE",   "LD   (@w),DE",	"(NEG)",		"(RETN)",		"IM   1",       "LD   R,A",
	"IN   E,(C)",   "OUT  (C),E",   "ADC  HL,DE",   "LD   DE,(@w)",	"(NEG)",		"(RETN)",		"IM   2",   	"LD   A,R",
	"IN   H,(C)",   "OUT  (C),H",   "SBC  HL,HL",   "LD   (@w),HL",	"(NEG)",		"(RETN)",		"(IM  0)",      "RRD",
	"IN   L,(C)",   "OUT  (C),L",   "ADC  HL,HL",   "LD   HL,(@w)",	"(NEG)",		"(RETN)",		"(IM  0)",   	"RLD",
	"IN   (C)",		"OUT  (C),0",   "SBC  HL,SP",   "LD   (@w),SP",	"(NEG)",		"(RETN)",		"(IM  0)",      "(NOP)",
	"IN   A,(C)",   "OUT  (C),A",   "ADC  HL,SP",   "LD   SP,(@w)",	"(NEG)",		"(RETN)",		"(IM  0)",   	"(NOP)",
};

const char* Z80Diag::InstED2[16] = 
{
	"LDI",	"CPI",	"INI",	"OUTI",	"LDD",	"CPD",	"IND",	"OUTD",
	"LDIR",	"CPIR",	"INIR",	"OTIR",	"LDDR",	"CPDR",	"INDR",	"OTDR",
};

const char* Z80Diag::InstCB[32] = 
{
	"RLC  ",	"RRC  ",	"RL   ",	"RR   ",	"SLA  ",	"SRA  ",	"SLL  ",	"SRL  ",
	"BIT  0,",	"BIT  1,",	"BIT  2,",	"BIT  3,",	"BIT  4,",	"BIT  5,",	"BIT  6,",	"BIT  7,",
	"RES  0,",	"RES  1,",	"RES  2,",	"RES  3,",	"RES  4,",	"RES  5,",	"RES  6,",	"RES  7,",
	"SET  0,",	"SET  1,",	"SET  2,",	"SET  3,",	"SET  4,",	"SET  5,",	"SET  6,",	"SET  7,",
};

const int8 Z80Diag::SizeTable[256] =
{
	 1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	 2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
	 2, 3, 3, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1,
	 2, 3, 3, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1,
	 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1, 1, 1,-1, 1,
	 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1, 1, 1,-1, 1,
	 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1, 1, 1,-1, 1,
	-1,-1,-1,-1,-1,-1, 1,-1, 1, 1, 1, 1, 1, 1,-1, 1,
	 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1, 1, 1,-1, 1,
	 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1, 1, 1,-1, 1,
	 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1, 1, 1,-1, 1,
	 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1, 1, 1,-1, 1,
	 1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 2, 3, 3, 2, 1,
	 1, 1, 3, 2, 3, 1, 2, 1, 1, 1, 3, 2, 3, 0, 2, 1,
	 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 0, 2, 1,
	 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 0, 2, 1,
};

const int8 Z80Diag::SizeTableED[128] =
{
	2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2,
	2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2,
	2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2,
	2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
};

