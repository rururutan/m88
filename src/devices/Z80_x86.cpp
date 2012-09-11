// ---------------------------------------------------------------------------
//	Z80 emulator for x86/VC6
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	$Id: Z80_x86.cpp,v 1.29 2003/05/19 02:33:56 cisc Exp $
// ---------------------------------------------------------------------------
//	注意:
//	VC6 以外でコンパイルすることは考えない方がいいと思う
//	#インテル最適化コンパイラでも通らないし(^^;;;
//	
//	Memory 関数は __cdecl, IO 関数は __stdcall (IOCALL) を想定しています
//

#include "headers.h"
#include "types.h"


#ifdef USE_Z80_X86

#include "Z80_x86.h"

//#define NO_UNOFFICIALFLAGS

#define FASTFETCH		1

// ---------------------------------------------------------------------------
//	Configuration
//	VC のないすな仕様により，
//	インラインアセンブリからクラス内定数にアクセスできないため，
//	ここで明示的に指定してやる必要がある（やれやれ）
//
#define PAGEBITS			10			// == MemoryManager::pagebits

#define IDBIT				PTR_IDBIT	// == MemoryManager::idbit

// ---------------------------------------------------------------------------
//	レジスタのわりあて
//		eax		A/F/R
//		ebx		work (PQ)
//		ecx		work (ST) / Address Register
//		edx		work (UV) / Data Register
//		esi		this (Z80_x86*)
//		edi		Instruction Pointer (PC/&Memory[PC])
//		ebp		Clock counter
//
extern Z80_x86* z80_ptr;		// dummy. 実際には存在しない

static Z80_x86* currentz80;

#define	CPU			[esi]z80_ptr

#define A			al
#define B			CPU.reg.r.b.b
#define C			CPU.reg.r.b.c
#define D			CPU.reg.r.b.d
#define E			CPU.reg.r.b.e
#define H			CPU.reg.r.b.h
#define L			CPU.reg.r.b.l
#define XH			CPU.reg.r.b.xh
#define XL			CPU.reg.r.b.xl
#define YH			CPU.reg.r.b.yh
#define YL			CPU.reg.r.b.yl
#define SPH			CPU.reg.r.b.sph
#define SPL			CPU.reg.r.b.spl
#define BC			CPU.reg.r.w.bc
#define DE			CPU.reg.r.w.de
#define HL			CPU.reg.r.w.hl
#define IX			CPU.reg.r.w.ix
#define IY			CPU.reg.r.w.iy
#define SP			CPU.reg.r.w.sp
#define I			CPU.reg.ireg
#define R			CPU.reg.rreg		// R の 第６ビットまで
#define R7			CPU.reg.rreg7		// R の 第７ビット
#define IFF1		CPU.reg.iff1
#define IFF2		CPU.reg.iff2
#define REVAF		CPU.reg.r_af		// 裏
#define REVBC		CPU.reg.r_bc
#define REVDE		CPU.reg.r_de
#define REVHL		CPU.reg.r_hl
#define FLAGX		CPU.flagn			// 「未使用」フラグ


#define INTMODE		CPU.reg.intmode
#define WAITSTATE	CPU.waitstate
#define CLOCKCOUNT	ebp

#define BUS			CPU.bus
#define RDPAGES		CPU.rdpages
#define WRPAGES		CPU.wrpages
#define WAITTBL		CPU.waittable

#define INST		edi
#define INSTBASE	CPU.instbase		// INSTBASE = PC - INST
#define INSTPAGE	CPU.instpage
#define INSTLIM		CPU.instlim
#define INSTWAIT	CPU.instwait

#define P			bh
#define Q			bl
#define	PQ			ebx
#define S			ch
#define T			cl
#define ST			ecx
#define U			dh
#define V			dl
#define UV			edx

#define CF			0x01
#define NF			0x02
#define PF			0x04
#define HF			0x10
#define ZF			0x40
#define SF			0x80

// ---------------------------------------------------------------------------
//	基本操作
//
#define SETF(flgs)			__asm { or ah,(flgs) }
#define CLRF(flgs)			__asm { and ah,not (flgs) }

#define LOADF				__asm { sahf }
#define LOADCY				__asm { ror ah,1 }
#define SAVEF				__asm { lahf }
#define SAVEFX(reg)			__asm { mov FLAGX,reg } \
							SAVEF 

#define MOV(dest, src)		__asm { mov V, src } \
							__asm { mov dest, V }
#define MOVD(dest, src)		__asm { mov dest, src }
#define MOVN(dest, n)		__asm { mov dest, n }
#define MOV16(dest, src)	__asm { mov dest, src }
#define MOV16N(dest, n)		__asm { mov dest, n }
#define INC16(reg)			__asm { inc reg }
#define DEC16(reg)			__asm { dec reg }
#define ADD16(d, s)			__asm { add d, s }
#define ADD16N(reg, n)		__asm { add reg, n } 
#define SUB16(d, s)			__asm { sub d, s }
#define SUB16N(reg, n)		__asm { sub reg, n } 
#define XCHG16(r1, r2)		__asm { xchg r1, r2 }
#define MOVSX(dest, src)	__asm { movsx dest,src }
#define SETIM(n)			__asm { mov INTMODE, n }
#define INTERRUPT			__asm { call O_INTR }
#define OINTERRUPT			__asm { call O_OUTINTR }
#define CLK(n, m)			__asm { add CLOCKCOUNT,n }

#define SYNC				__asm { call Sync }

// ---------------------------------------------------------------------------
//	PC 操作
//

#if FASTFETCH
	#define SETPC(d)			__asm { mov INST, d } \
								__asm { call SetPC }

	#define GETPC(reg)			MOV16(reg, INSTBASE); ADD16(reg, INST) 

	#define PCINC1				INC16(INST)
	#define PCINC2				ADD16N(INST, 2)

	#define PCDEC_(id)			__asm { cmp INST, INSTPAGE } \
								__asm { jb pcdec_indirect_##id } \
								__asm { ret } \
								pcdec_indirect_##id: \
								__asm { add INST, INSTBASE } \
								__asm { jmp SetPC }
	
	#define PCDEC1				DEC16(INST) PCDEC_(0)
	#define PCDEC1a				DEC16(INST) PCDEC_(1)
	#define PCDEC2				SUB16(INST,2) PCDEC_(2)

	#define MJUMP				SUB16(INST, INSTBASE) \
								PCDEC_(3)

	#define MJUMPR(r)			ADD16(INST, r) \
								PCDEC_(4)

#else

	#define SETPC(d)			MOV16(INST, d)			

	#define GETPC(reg)			MOV16(reg, INST)

	#define PCINC1				INC16(INST)
	#define PCINC2				ADD16N(INST, 2)

	#define PCDEC1				__asm { dec INST } __asm { ret }
	#define PCDEC1a				__asm { dec INST } __asm { ret } 
	#define PCDEC2				__asm { sub INST, 2 } __asm { ret }

	#define MJUMP				__asm { ret }				
	#define MJUMPR(r)			ADD16(INST, r)  __asm { ret }

#endif

// ---------------------------------------------------------------------------

#define PAGEMASK			((1 << (16 - PAGEBITS)) - 1)
#define PAGEOFFS			((1 << PAGEBITS) - 1)
#define PAGESHIFT			3

void O_INTR();
void O_OUTINTR();

// ---------------------------------------------------------------------------
//	高速読み込みのための PC 変換
//	arg:	edi(INST)	PC
//	ret:	edi(INST)	INST
//			ZF			direct if NZ
//	uses:	ecx, edx
//
static void __declspec(naked) SetPC()
{
	__asm 
	{
#if FASTFETCH
		mov edx,INST
		shr edx,PAGEBITS - 2
		and edx,PAGEMASK << 2
		
		mov ecx,WAITTBL[edx]
		mov edx,RDPAGES[edx * (1 << (PAGESHIFT-2))].ptr
		mov INSTWAIT,ecx
		test edx,IDBIT
		jnz indirect
		
	// instruction is on memory
	//	ecx = page->read
	//	edi = newpc
	//	instpage = ((uint8*) page->read);
	//	instbase = -((uint8*) page->read) + (newpc & ~MemoryManager::pagemask & 0xffff);
	//	instlim = ((uint8*) page->read) + (1 << MemoryManager::pagebits);
	//	inst = ((uint8*) page->read) + (newpc & MemoryManager::pagemask);
		mov INSTPAGE, edx
		
		mov ecx, INST
		and ecx, (~PAGEOFFS) & 0ffffh
		sub ecx, edx
		
		and INST, PAGEOFFS
		add INST, edx
		mov INSTBASE, ecx

		add edx, 1 << PAGEBITS
		mov INSTLIM, edx
		ret
#endif		
	indirect:
	// instruction is not on memory
	//	instbase = instlim = 0;
	//	instpage = (uint8*) ~0;
	//	inst = (uint8*) newpc;
		or edx,-1
		xor ecx,ecx
		mov INSTPAGE,edx
		mov INSTBASE,ecx
		mov INSTLIM,ecx
		ret
	}
}

// ---------------------------------------------------------------------------
//	同期チェック
//	arg:	edx = SyncPort
//			ebx = -check clock
//	ret:	CF = Need synchonization
//	use:	ebx
//
static void __declspec(naked) Sync()
{
	__asm
	{
		push ecx
		and edx,0ffh
		mov ecx,CPU.ioflags
		test byte ptr [ecx+edx],-1
		jnz sync_test
		pop ecx
		ret

	sync_test:
		push edx
		neg ebx			// ebx = clockcount
		
		mov cl,CPU.eshift
		sal ebx,cl
		
		mov edx,CPU.execcount
		add edx,ebx				// ebp
		cmp CPU.delaycount,edx	// (count1-1 - count2 >= 0) ? 
		
		// (delaycount - GetCount() < 0) なら後で実行する
		pop edx
		js sync_needed
		pop ecx
		or ebx,ebx
		ret
	
	sync_needed:
		add CPU.execcount, ebx
		xor CLOCKCOUNT, CLOCKCOUNT
		pop ecx
		stc
		ret
	}
}


// ---------------------------------------------------------------------------
//	インターフェースマクロ
//
//	マクロ名			破壊してもよいレジスタ	機能
//	READ8				ebx ecx edx				edx = byte[cx]
//	READ16				ebx ecx edx				edx = word[cx]
//	WRITE8				ebx ecx edx				[cx] = dl
//	WRITE16				ebx ecx edx				[cx] = dx
//	FETCH				ecx edx					edx = byte[PC], PC++
//	FETCH16				ecx edx					edx = word[PC], PC++
//
//	INP					ebx ecx edx				edx = In(ecx)
//	OUTP				ebx ecx edx				Out(ecx, edx)

// ---------------------------------------------------------------------------
//	１バイト読み込み
//	arg:	ecx(ST)		address
//	ret:	dl(edx)(V)	data
//	uses:	ebx(PQ), ecx(ST), edx(UV)
//
//	equiv:	edx = MemoryManager::Read8(ecx)
//
static void __declspec(naked) Reader8()
{
	__asm
	{
		shld edx,ecx,32-(PAGEBITS-2)
		and edx,PAGEMASK << 2
		mov ebx,RDPAGES[edx * (1 << (PAGESHIFT-2))].ptr
		add CLOCKCOUNT,WAITTBL[edx]
		test ebx,IDBIT
		jnz indirect
		
//	direct:
		and ecx,PAGEOFFS
		movzx edx,byte ptr [ebx+ecx]
		ret
		
	indirect:
		and ecx,0ffffh
		push eax
		mov edx,RDPAGES[edx * (1 << (PAGESHIFT-2))].inst
		push ecx				// ADDR
		and ebx,not IDBIT
		push edx				// INST
		call ebx
		mov edx,eax 
		pop eax
		ret
	}
}


// ---------------------------------------------------------------------------
//	命令１バイト読み込み
//	ret:	dl(edx)	data
//	uses:	ecx, edx
//
//	equiv:	edx = MemoryManager::Read8(PC++)
//
static void __declspec(naked) Fetcher8()
{
	__asm
	{
		cmp INST,INSTLIM
		jnb indirect
		add CLOCKCOUNT,INSTWAIT
		movzx edx,byte ptr [INST]
		inc INST
		ret

	indirect:
		add INST,INSTBASE		// edi を PC に変換
		call SetPC
		jz func
		
		add CLOCKCOUNT,INSTWAIT
		movzx edx,byte ptr [INST]
		inc INST
		ret

	func:
		add CLOCKCOUNT,INSTWAIT
		shld edx,INST,32-(PAGEBITS-PAGESHIFT)
		and INST,0ffffh
		and edx,PAGEMASK << PAGESHIFT
		mov ecx,RDPAGES[edx].ptr
		push eax
		mov edx,RDPAGES[edx].inst
		push INST				// ADDR
		and ecx,not IDBIT
		inc INST
		push edx				// INST
		call ecx
		mov edx,eax 
		pop eax
		ret
	}
}
		
// ---------------------------------------------------------------------------
//	命令１バイト読み込み(2)
//	ret:	edx		data(sign extended)
//	uses:	ecx, edx
//
//	equiv:	edx = (int8) MemoryManager::Read8(PC++)
//
static void __declspec(naked) Fetcher8sx()
{
	__asm
	{
		cmp INST,INSTLIM
		jnb indirect
		add CLOCKCOUNT,INSTWAIT
		movsx edx,byte ptr [INST]
		inc INST
		ret

	indirect:
		add INST,INSTBASE		// edi を PC に変換
		call SetPC
		jz func
		
		add CLOCKCOUNT,INSTWAIT
		movsx edx,byte ptr [INST]
		inc INST
		ret

	func:
		add CLOCKCOUNT,INSTWAIT
		shld edx,INST,32-(PAGEBITS-PAGESHIFT)
		and INST,0ffffh
		and edx,PAGEMASK << PAGESHIFT
		mov ecx,RDPAGES[edx].ptr
		push eax
		mov edx,RDPAGES[edx].inst
		push INST				// ADDR
		and ecx,not IDBIT
		inc INST
		push edx				// INST
		call ecx
		movsx edx,al 
		pop eax
		ret
	}
}

// ---------------------------------------------------------------------------
//	命令２バイト読み込み
//	ret:	dl(edx)	data
//	uses:	ecx, edx
//
//	equiv:	edx = MemoryManager::Read16(PC), PC+=2
//
static void __declspec(naked) Fetcher16()
{
	__asm
	{
		mov ecx,INSTWAIT
		add INST,2
		lea CLOCKCOUNT,[CLOCKCOUNT+ecx*2]
		cmp INST,INSTLIM
		ja indirect
		movzx edx,word ptr [INST-2]
		ret
	
	indirect:
		sub INST,2
		push ebx
		call Fetcher8
		mov ebx,edx
		call Fetcher8
		shl edx,8
		or edx,ebx
		pop ebx
		ret
	}
}

// ---------------------------------------------------------------------------
//	２バイト読み込み
//	arg:	ecx(ST)		address
//	ret:	dx(edx)(V)	data
//	uses:	ebx, ecx, edx (PQSTUV)
//
//	equiv:	edx = MemoryManager::Read16(ecx)
//
static void __declspec(naked) Reader16()
{
	__asm
	{
		lea edx,[ecx+1]
		test edx,PAGEOFFS
		jz boundary
		
		shr ecx,PAGEBITS-2
		and ecx,PAGEMASK << 2
		mov ebx,RDPAGES[ecx * (1 << (PAGESHIFT-2))].ptr
		test ebx,IDBIT
		jnz indirect
		
		and edx,PAGEOFFS
		mov ecx,WAITTBL[ecx]
		movzx edx,word ptr [ebx+edx-1]
		lea CLOCKCOUNT,[CLOCKCOUNT+ecx*2]
		ret
		
	indirect:
		and edx,0ffffh
		push eax
		mov eax,RDPAGES[ecx * (1 << (PAGESHIFT-2))].inst
		push edx			// addr
		dec edx
		and edx,0ffffh
		push eax			// inst
		push edx			// addr
		push eax			// inst
		and ebx,not IDBIT
		call ebx			// 1st
		xchg eax,ebx
		call eax			// 2nd
		shl eax,8
		lea edx,[eax+ebx]
		pop eax
		ret
		
	boundary:
		push ecx
		call Reader8
		pop ecx
		inc ecx
		push edx
		call Reader8
		pop ecx
		shl edx,8
		or edx,ecx
		ret
	}
}

// ---------------------------------------------------------------------------
//	１バイト書き込み
//	arg:	dl		data
//			ecx		address
//	uses:	ebx, ecx, edx
//
//	equiv:	MemoryManager::Write8(ecx, dl)
//
static void __declspec(naked) Writer8()
{
	__asm
	{
		mov ebx,ecx
		shr ecx,PAGEBITS - 2
		and ecx,PAGEMASK << 2
		add CLOCKCOUNT,WAITTBL[ecx]
		mov ecx,WRPAGES[ecx * (1 << (PAGESHIFT-2))].ptr
		test ecx,IDBIT
		jnz indirect
		
	// direct
		and ebx,PAGEOFFS
		mov [ecx+ebx],dl
		ret
	
	indirect:
		push eax
		and edx,0ffh
		and ebx,0ffffh
		push edx		// data
		and ecx,not IDBIT
		push ebx		// addr
		shr ebx,PAGEBITS - PAGESHIFT
		and ebx,PAGEMASK << PAGESHIFT
		mov ebx,WRPAGES[ebx].inst
		push ebx		// inst
		call ecx
		pop eax
		ret
	}
}

// ---------------------------------------------------------------------------
//	２バイト書き込み
//	arg:	dx		data
//			ecx		address
//	uses:	ebx, ecx, edx
//
//	equiv:	MemoryManager::Write16(ecx, dx)
//
static void __declspec(naked) Writer16()
{
	__asm
	{
		lea ebx,[ecx+1]
		test ebx,PAGEOFFS
		jz boundary
		
		push edx
		shr ecx,PAGEBITS - 2
		and ecx,PAGEMASK << 2
		mov edx,WRPAGES[ecx * (1 << (PAGESHIFT-2))].ptr
		test edx,IDBIT
		jnz indirect
		// b:addr c:page d:write
		
	// direct
		mov ecx,WAITTBL[ecx]
		and ebx,PAGEOFFS
		lea CLOCKCOUNT,[CLOCKCOUNT+ecx*2]
		pop ecx
		mov [edx+ebx-1],cx
		ret
	
	indirect:
		push eax
		mov eax,WAITTBL[ecx]	
		and ebx,0ffffh
		lea CLOCKCOUNT,[CLOCKCOUNT+eax*2]
		mov eax,[esp][4]
		shr eax,8
		push eax
		mov ecx,WRPAGES[ecx * (1 << (PAGESHIFT-2))].inst
		push ebx
		lea eax,[ebx-1]
		mov ebx,edx
		push ecx
		mov edx,[esp][16]
		and edx,0ffh
		push edx
		and eax,0ffffh
		push eax
		and ebx,not IDBIT
		push ecx
		call ebx
		call ebx
		pop eax
		add esp,4
		ret
	
	boundary:
		push edx
		push ecx
		call Writer8
		pop ecx
		inc ecx
		pop edx
		shr edx,8
		jmp Writer8
	}
}

#define READ8			__asm { call Reader8 }
#define READ16			__asm { call Reader16 }
#define WRITE8			__asm { call Writer8 }
#define WRITE16			__asm { call Writer16 }

#if FASTFETCH
	#if 0
		#define FETCH8			__asm { call Fetcher8 }
		#define FETCH8SUB		
	#else
		#define FETCH8			__asm { cmp INST,INSTLIM } \
								__asm { jnb fetch8_indirect } \
								__asm { movzx edx,byte ptr [INST] } \
								__asm { inc INST } \
								fetch8_end:
		#define FETCH8SUB		fetch8_indirect: \
								__asm { call Fetcher8 } \
								__asm { jmp fetch8_end }
	#endif

	#define FETCH8SX		__asm { call Fetcher8sx }
	#define FETCH16			__asm { call Fetcher16 }
#else
	#define FETCH8			__asm { push ebx } \
							__asm { mov ecx, edi } \
							__asm { inc edi } \
							__asm { call Reader8 } \
							__asm { pop ebx }

	#define FETCH8SUB

	#define FETCH8SX		__asm { push ebx } \
							__asm { mov ecx, edi } \
							__asm { inc edi } \
							__asm { call Reader8 } \
							__asm { movsx edx,dl } \
							__asm { pop ebx }


	#define FETCH16			__asm { push ebx } \
							__asm { mov ecx, edi } \
							__asm { add edi,2 } \
							__asm { call Reader16 } \
							__asm { pop ebx }
#endif

// ---------------------------------------------------------------------------
//	Bus I/O ------------------------------------------------------------------

//	Port[edx] <- ecx ---------------------------------------------------------

static void __declspec(naked) Bus_Out()
{
	__asm
	{
		push eax
		push esi
		mov CPU.clockcount,CLOCKCOUNT	// ebp
		
		and ecx,0ffh
		and edx,0ffh
		mov ebp,ecx				// ebp = data
		mov ecx,CPU.outs
		lea eax,[edx+edx*2]
		mov ebx,edx				// ebx = port
		lea esi,[ecx+eax*4]		// esi = inbank

looop:
		push ebp
		mov ecx,[esi]			// OutBank.device
		push ebx
		push ecx
		call [esi+4]			// OutBank.func
		
		mov esi,[esi+8]			// OutBank.next
		test esi,esi
		jnz looop

		pop esi
		pop eax
		mov CLOCKCOUNT,CPU.clockcount
		add INST,INSTBASE
		call SetPC
	}
	__asm { ret }
}

// ---------------------------------------------------------------------------
//	Port[edx] -> ecx ---------------------------------------------------------

static void __declspec(naked) Bus_In()
{
	__asm
	{
		push eax
		push esi
		mov CPU.clockcount,CLOCKCOUNT
		
		mov ecx,CPU.ins
		lea eax,[edx+edx*2]
		mov ebx,edx				// ebx = port
		lea esi,[ecx+eax*4]		// esi = inbank

		mov ebp,255
looop:
		mov ecx,[esi]			// InBank.device
		push ebx
		push ecx
		call [esi+4]			// InBank.func
		mov esi,[esi+8]			// InBank.next
		and ebp,eax
		test esi,esi
		jnz looop

		pop esi
		pop eax
		mov ecx,ebp
		mov CLOCKCOUNT,CPU.clockcount
		ret
	}
}

#define INP				__asm { and edx,0ffh } \
						__asm { call Sync } \
						__asm { jc inp_sync } \
						__asm { call Bus_In }
		
#define	OUTP			__asm { call Bus_Out }

// ---------------------------------------------------------------------------
//  スタック操作
//	STACK <-> UV
//
#define MPUSHUV			MOV16(ST, SP); SUB16N(ST, 2); MOV16(SP, ST); WRITE16
#define MPOPUV			MOV16(ST, SP); READ16; ADD16N(SP, 2);

// ---------------------------------------------------------------------------
//	IX+d / IY+d	アドレス算出 -> ST
//
#define LEAHL			MOV16(ST, HL)
#define LEAIX			FETCH8SX; MOV16(ST, IX); ADD16(ST, UV)
#define LEAIY			FETCH8SX; MOV16(ST, IY); ADD16(ST, UV)

// ---------------------------------------------------------------------------
//	レジスタ操作(特殊)
//	UV <-> AF, A <- R

//	AF->UV
#ifndef NO_UNOFFICIALFLAGS
#define LOADUVAF			__asm { mov V,FLAGX } \
							__asm { and ah,0d7h } \
							__asm { and V,28h } \
							__asm { mov U,A } \
							__asm { or  V,ah } 
#else
#define LOADUVAF			__asm { and ah,0d7h } \
							__asm { mov U,A } \
							__asm { mov V,ah } 
#endif
		
// UV->AF	
#define STOREAFUV			__asm { mov ah,V } \
							__asm { mov FLAGX,V } \
							__asm { mov A,U }

// 						
#define LOADAR				__asm { mov ecx,eax } \
							__asm { shr ecx,24 } \
							__asm { mov A,R7 } \
							__asm { and cl,7fh } \
							__asm { and A,80h } \
							__asm { or A,cl }

//
#define STORERA				__asm { mov ecx,eax } \
							__asm { and eax,00ffffffh } \
							__asm { shl ecx,24 } \
							__asm { mov R7, A } \
							__asm { or eax,ecx }


// ---------------------------------------------------------------------------
//	フラグ操作
//

#define USEVFP				__asm { seto T } \
							CLRF(PF+NF) \
							__asm { shl T,2 } \
							__asm { or ah,T }

#define USEVFM				__asm { seto T } \
							CLRF(PF) \
							__asm { shl T,2 } \
							SETF(NF) \
							__asm { or ah,T }

// A の内容と IFF から フラグセット

#define TESTIFF				LOADF \
							__asm { inc A } \
							__asm { dec A } \
							SAVEFX(A) \
							__asm { test IFF1,-1 } \
							__asm { setnz V } \
							CLRF(HF+PF+NF) \
							__asm { shl V,2 } \
							__asm { or ah,V }

// ---------------------------------------------------------------------------
//	命令フロー操作
//
#define PUSH(r)				__asm { push r }
#define POP(r)				__asm { pop r }

#define MCALLUV				PUSH(UV); GETPC(UV); MPUSHUV; POP(INST); MJUMP
#define MRET				MPOPUV; MOV16(INST, UV); MJUMP

// ---------------------------------------------------------------------------
// 	８ビット算術演算
//

#define MADD_A(reg)			__asm { add A,reg } \
							SAVEFX(A) \
							USEVFP

#define MADC_A(reg)			LOADCY \
							__asm { adc A,reg } \
							SAVEFX(A) \
							USEVFP
							
#define MSUB(reg)			__asm { sub A,reg } \
							SAVEFX(A) \
							USEVFM 

#define MSBC_A(reg)			LOADCY \
							__asm { sbb A,reg } \
							SAVEFX(A) \
							USEVFM
							
#define MAND(reg)			__asm { and A,reg } \
							SAVEFX(A) \
							CLRF(NF) \
							SETF(HF)
							
#define MOR(reg)			__asm { or A,reg } \
							SAVEFX(A) \
							CLRF(NF+HF)

#define MXOR(reg)			__asm { xor A,reg } \
							SAVEFX(A) \
							CLRF(NF+HF)
							
#define MCP(reg)			__asm { mov U,reg } \
							__asm { cmp A,U } \
							SAVEFX(U) \
							USEVFM
							
#define MINCV				LOADF \
							__asm { inc V } \
							SAVEFX(V) \
							USEVFP

#define MDECV				LOADF \
							__asm { dec V } \
							SAVEFX(V) \
							USEVFM 

// ---------------------------------------------------------------------------
//	１６ビット算術演算
//

#define MADD16(ph,pl,qh,ql)	__asm { mov T,ah } \
							__asm { add pl,ql } \
							__asm { adc ph,qh } \
							__asm { lahf } \
							__asm { and T,not (HF+NF+CF) } \
							__asm { and ah,HF+CF } \
							__asm { or ah,T }

// UV<-HL
#define MADCHL(rh,rl)		__asm { mov UV,HL } \
							LOADCY  \
							__asm { adc V,rl } \
							__asm { adc U,rh } \
							SAVEFX(U) \
							USEVFP \
							CLRF(ZF) \
							__asm { test UV,0ffffh } \
							__asm { mov HL,UV } \
							__asm { setz V } \
							__asm { shl V,6 } \
							__asm { or ah,V }

#define MSBCHL(rh,rl)		__asm { mov UV,HL } \
							LOADCY \
							__asm { sbb V,rl } \
							__asm { sbb U,rh } \
							SAVEFX(U) \
							USEVFM \
							CLRF(ZF) \
							__asm { test UV,0ffffh } \
							__asm { mov HL,UV } \
							__asm { setz V } \
							__asm { shl V,6 } \
							__asm { or ah,V }

// ---------------------------------------------------------------------------
//	ローテート・シフト
// 

#define MRLCV				__asm { rol V,1 } \
							__asm { dec V } \
							__asm { inc V } \
							SAVEFX(V) \
							CLRF(NF+HF)
							
#define MRRCV				__asm { ror V,1 } \
							__asm { dec V } \
							__asm { inc V } \
							SAVEFX(V) \
							CLRF(NF+HF)
							
#define MRLV				LOADCY \
							__asm { adc V,V } \
							SAVEFX(V) \
							CLRF(NF+HF) 

#define MRRV				LOADF \
							__asm { rcr V,1 } \
							__asm { dec V } \
							__asm { inc V } \
							SAVEFX(V) \
							CLRF(NF+HF)

#define MSLAV				__asm { sal V,1 } \
							__asm { dec V } \
							__asm { inc V } \
							SAVEFX(V) \
							CLRF(NF+HF) 
							
#define MSRAV				__asm { sar V,1 } \
							__asm { dec V } \
							__asm { inc V } \
							SAVEFX(V) \
							CLRF(NF+HF)
							
#define MSLLV				__asm { stc } \
							__asm { adc V,V } \
							SAVEFX(V) \
							CLRF(NF+HF)
							
#define MSRLV				__asm { shr V,1 } \
							__asm { dec V } \
							__asm { inc V } \
							SAVEFX(V) \
							CLRF(NF+HF)

// ---------------------------------------------------------------------------
//	RLD/RRD ------------------------------------------------------------------

#define MRLD				MOV16(ST, HL) \
							READ8 \
							__asm { mov T,A } \
							__asm { mov U,V } \
							__asm { and A,0f0h } \
							__asm { shr U,4 } \
							__asm { shl V,4 } \
							__asm { and T,0fh } \
							__asm { or A,U } \
							__asm { or V,T } \
							MOV16(ST, HL) \
							WRITE8; \
							LOADF \
							__asm { inc A } \
							__asm { dec A } \
							SAVEFX(A) \
							CLRF(NF+HF)
							
#define MRRD				MOV16(ST, HL); \
							READ8 \
							__asm { mov T,A } \
							__asm { mov U,V } \
							__asm { shr V,4 } \
							__asm { and U,0fh } \
							__asm { shl T,4 } \
							__asm { and A,0f0h } \
							__asm { or V,T } \
							__asm { or A,U } \
							MOV16(ST, HL) \
							WRITE8 \
							LOADF \
							__asm { inc A } \
							__asm { dec A } \
							SAVEFX(A) \
							CLRF(NF+HF)
		
// ---------------------------------------------------------------------------
//	条件分岐 -----------------------------------------------------------------

#define MIFNZ(dest)			__asm { test ah,ZF } __asm { jz dest }
#define MIFZ(dest)			__asm { test ah,ZF } __asm { jnz dest }
#define MIFNC(dest)			__asm { test ah,CF } __asm { jz dest }
#define MIFC(dest)			__asm { test ah,CF } __asm { jnz dest }
#define MIFPO(dest)			__asm { test ah,PF } __asm { jz dest }
#define MIFPE(dest)			__asm { test ah,PF } __asm { jnz dest }
#define MIFP(dest)			__asm { test ah,SF } __asm { jz dest }
#define MIFM(dest)			__asm { test ah,SF } __asm { jnz dest }
#define MIFSUB(dest)		__asm { test ah,NF } __asm { jnz dest }

// ---------------------------------------------------------------------------
//	単体命令エミュレーションマクロ
//

// ---------------------------------------------------------------------------
//  あきゅみゅれーた・ローテートシフト ----------------------------------------

#define MRLCA				LOADF \
							__asm { rol A,1 } \
							SAVEFX(A) \
							CLRF(NF+HF)

#define MRRCA				LOADF \
							__asm { ror A,1 } \
							SAVEFX(A) \
							CLRF(NF+HF) 

#define MRLA				LOADF \
							__asm { rcl A,1 } \
							SAVEFX(A) \
							CLRF(NF+HF)

#define MRRA				LOADF \
							__asm { rcr A,1 } \
							SAVEFX(A) \
							CLRF(NF+HF) 

// ---------------------------------------------------------------------------
//	あきゅみゅれーた 操作命令 -------------------------------------------------

#define MDAA				__asm { mov cl,ah } \
							__asm { and cl,NF } \
							__asm { jnz M_DAA_1 } \
							LOADF \
							__asm { daa } \
							__asm { jmp M_DAA_2 } \
							M_DAA_1: \
							LOADF \
							__asm { das } \
							M_DAA_2: \
							SAVEFX(A) \
							__asm { and ah,not NF } \
							__asm { or ah,cl }

#define MCPL				__asm { not A } \
							__asm { mov FLAGX,A } \
							SETF(HF+NF) 
							
#define MSCF				__asm { mov FLAGX,A } \
							SETF(CF) \
							CLRF(NF+HF) \

#define MCCF				__asm { mov FLAGX,A } \
							__asm { mov V,ah } \
							__asm { and ah,~(NF|HF) } \
							__asm { and V,1 } \
							__asm { xor ah,CF } \
							__asm { shl V,4 } \
							__asm { or ah,V }



// ---------------------------------------------------------------------------
//	ブロック入出力操作 -------------------------------------------------------

#define MINX 				MOV16(UV, BC) \
							INP \
							MOVD(V, T) \
							MOV16(ST, HL) \
							WRITE8 \
							LOADF \
							__asm { dec B } \
							SAVEF \
							SETF(NF)

#define MOUTX 				MOV16(UV, BC) \
							SYNC \
							__asm { jc outx_sync } \
							MOV16(ST, HL) \
							READ8 \
							MOVD(T, V) \
							MOV16(UV, BC) \
							OUTP \
							LOADF \
							__asm { dec B } \
							SAVEF \
							SETF(NF) \
							OINTERRUPT

// ---------------------------------------------------------------------------
//	ブロック転送操作 ---------------------------------------------------------

#define MLDI				__asm { mov ST, HL } \
							READ8 \
							__asm { mov ST, DE } \
							WRITE8 \
							__asm { inc HL } \
							__asm { mov ST, BC } \
							__asm { inc DE } \
							__asm { dec ST } \
							__asm { test ST,0ffffh } \
							__asm { mov BC,ST } \
							__asm { setnz V } \
							CLRF(HF+PF+NF) \
							__asm { shl V,2 } \
							__asm { or ah,V } 

#define MLDD				__asm { mov ST, HL } \
							READ8 \
							__asm { mov ST, DE } \
							WRITE8 \
							__asm { dec HL } \
							__asm { mov ST, BC } \
							__asm { dec DE } \
							__asm { dec ST } \
							__asm { test ST,0ffffh } \
							__asm { mov BC,ST } \
							__asm { setnz V } \
							CLRF(HF+PF+NF) \
							__asm { shl V,2 } \
							__asm { or ah,V } 

// ---------------------------------------------------------------------------
//	ブロックサーチ操作 -------------------------------------------------------

#define MCPI				__asm { mov ST, HL } \
							__asm { lea UV, [ST+1] } \
							__asm { mov HL, UV } \
							READ8 \
							__asm { mov U, ah } \
							LOADF \
							__asm { cmp A, V } \
							SAVEF \
							__asm { and U, 1 } \
							CLRF(PF+CF) \
							__asm { or U,NF } \
							__asm { mov ST,BC } \
							__asm { dec ST } \
							__asm { test ST,0ffffh } \
							__asm { mov BC,ST } \
							__asm { setnz V } \
							__asm { or ah, U } \
							__asm { shl V, 2 } \
							__asm { or ah, V }

#define MCPD				__asm { mov ST, HL } \
							__asm { lea UV, [ST-1] } \
							__asm { mov HL, UV } \
							READ8 \
							__asm { mov U, ah } \
							LOADF \
							__asm { cmp A, V } \
							SAVEF \
							__asm { and U, 1 } \
							CLRF(PF+CF) \
							__asm { or U,NF } \
							__asm { mov ST,BC } \
							__asm { dec ST } \
							__asm { test ST,0ffffh } \
							__asm { mov BC,ST } \
							__asm { setnz V } \
							__asm { or ah, U } \
							__asm { shl V, 2 } \
							__asm { or ah, V }


// ---------------------------------------------------------------------------
//  コード部分
//
#define OPFUNC(label)	void __declspec(naked) O_##label ()
#define OPEND			__asm { ret }
typedef void (*OpFuncPtr)();

// ---------------------------------------------------------------------------
//	アキュムレータ操作命令命令 -----------------------------------------------

static OPFUNC(DAA) { MDAA; CLK(4, 1); OPEND; }
static OPFUNC(CPL) { MCPL; CLK(4, 1); OPEND; }
static OPFUNC(CCF) { MCCF; CLK(4, 1); OPEND; }
static OPFUNC(SCF) { MSCF; CLK(4, 1); OPEND; }

static OPFUNC(RLCA) { MRLCA; CLK(4, 1); OPEND; }
static OPFUNC(RRCA) { MRRCA; CLK(4, 1); OPEND; }
static OPFUNC(RLA)  { MRLA; CLK(4, 1); OPEND; }
static OPFUNC(RRA)  { MRRA; CLK(4, 1); OPEND; }

static OPFUNC(RLD)  { MRLD; CLK(18, 4); OPEND; }
static OPFUNC(RRD)  { MRRD; CLK(18, 4); OPEND; }

static OPFUNC(NEG) { MOVD(V, A); MOVN(A, 0); CLK(8, 2); MSUB(V); OPEND; }

// ---------------------------------------------------------------------------
//	IM -----------------------------------------------------------------------

static OPFUNC(IM0) { SETIM(0); CLK(8, 2); OPEND; }
static OPFUNC(IM1) { SETIM(1); CLK(8, 2); OPEND; }
static OPFUNC(IM2) { SETIM(2); CLK(8, 2); OPEND; }

// ---------------------------------------------------------------------------
//	入出力命令 ---------------------------------------------------------------

static OPFUNC(IN_A_N)
{
	__asm { mov PQ,INSTWAIT }
	__asm { sub PQ,CLOCKCOUNT }		// ST = -(prevcount)
	FETCH8;
//	MOVD(U, A); 
	INP;		// T <- (UV)
	MOVD(A, T); 
	CLK(11, 2); 
	OPEND;

	FETCH8SUB;
inp_sync:
	PCDEC2;
	OPEND;
}

static OPFUNC(OUT_N_A) 
{ 
	__asm { mov PQ,INSTWAIT }
	__asm { sub PQ,CLOCKCOUNT }		// ST = -(prevcount)
	FETCH8;
//	MOVD(U, A); 
	MOVD(T, A);
	SYNC
	__asm { jc outp_sync }

	OUTP;		// (UV) <- T 
	CLK(11, 2); 
	OINTERRUPT;
	OPEND; 
	FETCH8SUB;
outp_sync:
	PCDEC2;
	OPEND;
}

#define MINPC(reg)		MOV16(UV, BC) \
						INP \
						LOADF \
						__asm { mov reg,T } \
						__asm { inc T } \
						__asm { dec T } \
						SAVEFX(T) \
						CLRF(HF|NF) \
						CLK(12, 2) \
						OPEND \
						inp_sync: \
						PCDEC2; \
						OPEND

static OPFUNC(IN_B_C) { MINPC(B); }
static OPFUNC(IN_C_C) { MINPC(C); }
static OPFUNC(IN_D_C) { MINPC(D); }
static OPFUNC(IN_E_C) { MINPC(E); }
static OPFUNC(IN_H_C) { MINPC(H); }
static OPFUNC(IN_L_C) { MINPC(L); }
static OPFUNC(IN_F_C) { MINPC(T); }
static OPFUNC(IN_A_C) { MINPC(A); }

#define MOUTPC(reg)		MOV16(UV, BC) \
						SYNC \
						__asm { jc outp_sync } \
						MOVD(T, reg) \
						OUTP \
						CLK(12, 2) \
						OINTERRUPT \
						OPEND \
						outp_sync: \
						PCDEC2; \
						OPEND

static OPFUNC(OUT_C_B) { MOUTPC(B) }
static OPFUNC(OUT_C_C) { MOUTPC(C) }
static OPFUNC(OUT_C_D) { MOUTPC(D) }
static OPFUNC(OUT_C_E) { MOUTPC(E) }
static OPFUNC(OUT_C_H) { MOUTPC(H) }
static OPFUNC(OUT_C_L) { MOUTPC(L) }
static OPFUNC(OUT_C_Z) { MOUTPC(0) }
static OPFUNC(OUT_C_A) { MOUTPC(A) }
	
// ---------------------------------------------------------------------------
//	分岐命令 -----------------------------------------------------------------

static OPFUNC(JP) 	 { FETCH16; CLK(10, 3); MOV16(INST, UV); MJUMP; }
static OPFUNC(JR)    { FETCH8SX; CLK(12, 2); MJUMPR(UV); }

static OPFUNC(JP_NZ) { CLK(10, 3); MIFZ (skip); FETCH16; MOV16(INST, UV); MJUMP;  skip: PCINC2; OPEND; }
static OPFUNC(JP_Z)  { CLK(10, 3); MIFNZ(skip); FETCH16; MOV16(INST, UV); MJUMP;  skip: PCINC2; OPEND; }
static OPFUNC(JP_NC) { CLK(10, 3); MIFC (skip); FETCH16; MOV16(INST, UV); MJUMP;  skip: PCINC2; OPEND; }
static OPFUNC(JP_C)  { CLK(10, 3); MIFNC(skip); FETCH16; MOV16(INST, UV); MJUMP;  skip: PCINC2; OPEND; }
static OPFUNC(JP_PO) { CLK(10, 3); MIFPE(skip); FETCH16; MOV16(INST, UV); MJUMP;  skip: PCINC2; OPEND; }
static OPFUNC(JP_PE) { CLK(10, 3); MIFPO(skip); FETCH16; MOV16(INST, UV); MJUMP;  skip: PCINC2; OPEND; }
static OPFUNC(JP_P)  { CLK(10, 3); MIFM (skip); FETCH16; MOV16(INST, UV); MJUMP;  skip: PCINC2; OPEND; }
static OPFUNC(JP_M)  { CLK(10, 3); MIFP (skip); FETCH16; MOV16(INST, UV); MJUMP;  skip: PCINC2; OPEND; }

static OPFUNC(JR_NZ) { MIFZ (skip); FETCH8SX; CLK(12, 2); MJUMPR(UV);  skip: CLK(7, 2); PCINC1; OPEND; }
static OPFUNC(JR_Z)  { MIFNZ(skip); FETCH8SX; CLK(12, 2); MJUMPR(UV);  skip: CLK(7, 2); PCINC1; OPEND; }
static OPFUNC(JR_NC) { MIFC (skip); FETCH8SX; CLK(12, 2); MJUMPR(UV);  skip: CLK(7, 2); PCINC1; OPEND; }
static OPFUNC(JR_C)  { MIFNC(skip); FETCH8SX; CLK(12, 2); MJUMPR(UV);  skip: CLK(7, 2); PCINC1; OPEND; }

static OPFUNC(JP_HL) { MOV16(INST, HL); CLK(4, 1); MJUMP; }
static OPFUNC(JP_IX) { MOV16(INST, IX); CLK(4, 1); MJUMP; }
static OPFUNC(JP_IY) { MOV16(INST, IY); CLK(4, 1); MJUMP; }

static OPFUNC(CALL)    { FETCH16; CLK(17, 5); MCALLUV; }
static OPFUNC(CALL_NZ) { MIFZ (skip); FETCH16; CLK(17, 5); MCALLUV; skip: CLK(10, 3); PCINC2; OPEND; }
static OPFUNC(CALL_Z)  { MIFNZ(skip); FETCH16; CLK(17, 5); MCALLUV; skip: CLK(10, 3); PCINC2; OPEND; }
static OPFUNC(CALL_NC) { MIFC (skip); FETCH16; CLK(17, 5); MCALLUV; skip: CLK(10, 3); PCINC2; OPEND; }
static OPFUNC(CALL_C)  { MIFNC(skip); FETCH16; CLK(17, 5); MCALLUV; skip: CLK(10, 3); PCINC2; OPEND; }
static OPFUNC(CALL_PO) { MIFPE(skip); FETCH16; CLK(17, 5); MCALLUV; skip: CLK(10, 3); PCINC2; OPEND; }
static OPFUNC(CALL_PE) { MIFPO(skip); FETCH16; CLK(17, 5); MCALLUV; skip: CLK(10, 3); PCINC2; OPEND; }
static OPFUNC(CALL_P)  { MIFM (skip); FETCH16; CLK(17, 5); MCALLUV; skip: CLK(10, 3); PCINC2; OPEND; }
static OPFUNC(CALL_M)  { MIFP (skip); FETCH16; CLK(17, 5); MCALLUV; skip: CLK(10, 3); PCINC2; OPEND; }

static OPFUNC(RET)    { CLK(10, 3); MRET; }
static OPFUNC(RET_NZ) { MIFZ (skip); CLK(11, 3); MRET;  skip: CLK(5, 1); OPEND; }
static OPFUNC(RET_Z)  { MIFNZ(skip); CLK(11, 3); MRET;  skip: CLK(5, 1); OPEND; }
static OPFUNC(RET_NC) { MIFC (skip); CLK(11, 3); MRET;  skip: CLK(5, 1); OPEND; }
static OPFUNC(RET_C)  { MIFNC(skip); CLK(11, 3); MRET;  skip: CLK(5, 1); OPEND; }
static OPFUNC(RET_PO) { MIFPE(skip); CLK(11, 3); MRET;  skip: CLK(5, 1); OPEND; }
static OPFUNC(RET_PE) { MIFPO(skip); CLK(11, 3); MRET;  skip: CLK(5, 1); OPEND; }
static OPFUNC(RET_P)  { MIFM (skip); CLK(11, 3); MRET;  skip: CLK(5, 1); OPEND; }
static OPFUNC(RET_M)  { MIFP (skip); CLK(11, 3); MRET;  skip: CLK(5, 1); OPEND; }

static OPFUNC(RETN) { MOV(IFF1, IFF2); CLK(14, 4); MRET; }
static OPFUNC(RETI) { MOV(IFF1, IFF2); CLK(14, 4); MRET; }

static OPFUNC(DJNZ)
{
	__asm { mov V,B }
	__asm { dec V }
	__asm { jz no_jump }
	__asm { mov B,V }
	FETCH8SX;
	CLK(13, 2);
	MJUMPR(UV);
no_jump:
	CLK(8, 2);
	PCINC1;
	MOVD(B, V); 
	OPEND;
}

static OPFUNC(RST00) { CLK(11, 3); GETPC(UV); MPUSHUV; SETPC(0x00); OPEND; }
static OPFUNC(RST08) { CLK(11, 3); GETPC(UV); MPUSHUV; SETPC(0x08); OPEND; }
static OPFUNC(RST10) { CLK(11, 3); GETPC(UV); MPUSHUV; SETPC(0x10); OPEND; }
static OPFUNC(RST18) { CLK(11, 3); GETPC(UV); MPUSHUV; SETPC(0x18); OPEND; }
static OPFUNC(RST20) { CLK(11, 3); GETPC(UV); MPUSHUV; SETPC(0x20); OPEND; }
static OPFUNC(RST28) { CLK(11, 3); GETPC(UV); MPUSHUV; SETPC(0x28); OPEND; }
static OPFUNC(RST30) { CLK(11, 3); GETPC(UV); MPUSHUV; SETPC(0x30); OPEND; }
static OPFUNC(RST38) { CLK(11, 3); GETPC(UV); MPUSHUV; SETPC(0x38); OPEND; }

// ---------------------------------------------------------------------------
//	１６ビット算術命令 -------------------------------------------------------

static OPFUNC(ADDHL_BC) { MOV16(UV,HL); MADD16(U,V, B,C); MOVD(L, V); CLK(11, 1); MOVD(H, U); OPEND; }
static OPFUNC(ADDHL_DE) { MOV16(UV,HL); MADD16(U,V, D,E); MOVD(L, V); CLK(11, 1); MOVD(H, U); OPEND; }
static OPFUNC(ADDHL_HL) { MOV16(UV,HL); MADD16(U,V, U,V); MOVD(L, V); CLK(11, 1); MOVD(H, U); OPEND; }
static OPFUNC(ADDHL_SP) { MOV16(UV,HL); MADD16(U,V, SPH,SPL); MOVD(L, V); CLK(11, 1); MOVD(H, U); OPEND; }

static OPFUNC(ADDIX_BC) { MOV16(UV,IX); MADD16(U,V, B,C); CLK(11, 1); MOV16(IX,UV); OPEND; }
static OPFUNC(ADDIX_DE) { MOV16(UV,IX); MADD16(U,V, D,E); CLK(11, 1); MOV16(IX,UV); OPEND; }
static OPFUNC(ADDIX_IX) { MOV16(UV,IX); MADD16(U,V, U,V); CLK(11, 1); MOV16(IX,UV); OPEND; }
static OPFUNC(ADDIX_SP) { MOV16(UV,IX); MADD16(U,V, SPH,SPL); CLK(11, 1); MOV16(IX,UV); OPEND; }

static OPFUNC(ADDIY_BC) { MOV16(UV,IY); MADD16(U,V, B,C); CLK(11, 1); MOV16(IY,UV); OPEND; }
static OPFUNC(ADDIY_DE) { MOV16(UV,IY); MADD16(U,V, D,E); CLK(11, 1); MOV16(IY,UV); OPEND; }
static OPFUNC(ADDIY_IY) { MOV16(UV,IY); MADD16(U,V, U,V); CLK(11, 1); MOV16(IY,UV); OPEND; }
static OPFUNC(ADDIY_SP) { MOV16(UV,IY); MADD16(U,V, SPH,SPL); CLK(11, 1); MOV16(IY,UV); OPEND; }

// ADC/SBC HL,HL : 内部で UV <- HL であることを利用
static OPFUNC(ADCHL_BC) { CLK(15, 2); MADCHL(B,C); OPEND; }
static OPFUNC(ADCHL_DE) { CLK(15, 2); MADCHL(D,E); OPEND; }
static OPFUNC(ADCHL_HL) { CLK(15, 2); MADCHL(U,V); OPEND; }
static OPFUNC(ADCHL_SP) { CLK(15, 2); MADCHL(SPH,SPL); OPEND; }

static OPFUNC(SBCHL_BC) { CLK(15, 2); MSBCHL(B,C); OPEND; }
static OPFUNC(SBCHL_DE) { CLK(15, 2); MSBCHL(D,E); OPEND; }
static OPFUNC(SBCHL_HL) { CLK(15, 2); MSBCHL(U,V); OPEND; }
static OPFUNC(SBCHL_SP) { CLK(15, 2); MSBCHL(SPH,SPL); OPEND; }

#define INC16X(reg)  MOV16(UV, reg); INC16(UV); CLK(6, 1); MOV16(reg, UV)
static OPFUNC(INC_BC) { INC16X(BC); OPEND; }			// INC ss はフラグ変化なし
static OPFUNC(INC_DE) { INC16X(DE); OPEND; }
static OPFUNC(INC_HL) { INC16X(HL); OPEND; }
static OPFUNC(INC_SP) { INC16X(SP); OPEND; }
static OPFUNC(INC_IX) { INC16X(IX); OPEND; }
static OPFUNC(INC_IY) { INC16X(IY); OPEND; }
#undef INC16X

#define DEC16X(reg)  MOV16(UV, reg); DEC16(UV); CLK(6, 1); MOV16(reg, UV)
static OPFUNC(DEC_BC) { DEC16X(BC); OPEND; }			// DEC ss はフラグ変化なし
static OPFUNC(DEC_DE) { DEC16X(DE); OPEND; }
static OPFUNC(DEC_HL) { DEC16X(HL); OPEND; }
static OPFUNC(DEC_SP) { DEC16X(SP); OPEND; }
static OPFUNC(DEC_IX) { DEC16X(IX); OPEND; }
static OPFUNC(DEC_IY) { DEC16X(IY); OPEND; }
#undef DEC16X

// ---------------------------------------------------------------------------
//	交換命令 -----------------------------------------------------------------

static OPFUNC(EX_AF_AF) { LOADUVAF; MOV16(PQ, REVAF); MOV16(REVAF, UV); MOV16(UV, PQ); STOREAFUV; CLK(4, 1); OPEND; }
static OPFUNC(EX_SP_HL) { MPOPUV; MOV16(PQ, UV); MOV16(UV, HL); CLK(19, 5); MOV16(HL, PQ); MPUSHUV; OPEND; }
static OPFUNC(EX_SP_IX) { MPOPUV; MOV16(PQ, UV); MOV16(UV, IX); CLK(19, 5); MOV16(IX, PQ); MPUSHUV; OPEND; }
static OPFUNC(EX_SP_IY) { MPOPUV; MOV16(PQ, UV); MOV16(UV, IY); CLK(19, 5); MOV16(IY, PQ); MPUSHUV; OPEND; }

static OPFUNC(EX_DE_HL) { MOV16(UV, DE); MOV16(PQ, HL); CLK(4, 1); MOV16(HL, UV); MOV16(DE, PQ); OPEND; }

static OPFUNC(EXX) 
{ 
	MOV16(UV, REVBC); XCHG16(UV, BC); MOV16(REVBC, UV);
	MOV16(UV, REVDE); XCHG16(UV, DE); MOV16(REVDE, UV);
	MOV16(UV, REVHL); XCHG16(UV, HL); MOV16(REVHL, UV);
	CLK(4, 1); 
	OPEND;
}

// ---------------------------------------------------------------------------
//	CPU 制御命令 -------------------------------------------------------------

static OPFUNC(NOP) { CLK(4, 1); OPEND; }

static OPFUNC(HALT)
{
	__asm { mov WAITSTATE, 1 }
	__asm { test CPU.intr, -1 }
	__asm { jz halt_1 }
	CLK(48, 1);
	PCDEC1;
	__asm { ret }
halt_1:
	MOV16(CLOCKCOUNT, 0);
	PCDEC1a;
	__asm { ret }
}

// ---------------------------------------------------------------------------
//	8 bit 演算命令 -----------------------------------------------------------

#define	OPALU(func)			static OPFUNC(func##_B)  { M##func(B); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_C)  { M##func(C); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_D)  { M##func(D); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_E)  { M##func(E); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_H)  { M##func(H); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_L)  { M##func(L); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_A)  { M##func(A); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_XH) { M##func(XH); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_XL) { M##func(XL); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_YH) { M##func(YH); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_YL) { M##func(YL); CLK(4, 1); OPEND; } \
							static OPFUNC(func##_M)  { LEAHL; READ8; M##func(V); CLK(7, 2); OPEND; } \
							static OPFUNC(func##_MX) { LEAIX; READ8; M##func(V); CLK(15, 3); OPEND; } \
							static OPFUNC(func##_MY) { LEAIY; READ8; M##func(V); CLK(15, 3); OPEND; } \
							static OPFUNC(func##_N)  { FETCH8; M##func(V); CLK(7, 2); OPEND; FETCH8SUB; }

OPALU(ADD_A)	OPALU(ADC_A)	OPALU(SUB)	OPALU(SBC_A)
OPALU(AND)		OPALU(XOR)		OPALU(OR)	OPALU(CP)

static OPFUNC(INC_B)  { MOVD(V, B); MINCV; MOVD(B, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_C)  { MOVD(V, C); MINCV; MOVD(C, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_D)  { MOVD(V, D); MINCV; MOVD(D, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_E)  { MOVD(V, E); MINCV; MOVD(E, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_H)  { MOVD(V, H); MINCV; MOVD(H, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_L)  { MOVD(V, L); MINCV; MOVD(L, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_A)  { MOVD(V, A); MINCV; MOVD(A, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_XH) { MOVD(V, XH); MINCV; MOVD(XH, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_XL) { MOVD(V, XL); MINCV; MOVD(XL, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_YH) { MOVD(V, YH); MINCV; MOVD(YH, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_YL) { MOVD(V, YL); MINCV; MOVD(YL, V); CLK(4, 1); OPEND; }
static OPFUNC(INC_M)  { LEAHL; PUSH(ST); READ8; MINCV; POP(ST); WRITE8; CLK(11, 3); OPEND; }
static OPFUNC(INC_MX) { LEAIX; PUSH(ST); READ8; MINCV; POP(ST); WRITE8; CLK(19, 4); OPEND; }
static OPFUNC(INC_MY) { LEAIY; PUSH(ST); READ8; MINCV; POP(ST); WRITE8; CLK(19, 4); OPEND; }

static OPFUNC(DEC_B)  { MOVD(V, B); MDECV; MOVD(B, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_C)  { MOVD(V, C); MDECV; MOVD(C, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_D)  { MOVD(V, D); MDECV; MOVD(D, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_E)  { MOVD(V, E); MDECV; MOVD(E, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_H)  { MOVD(V, H); MDECV; MOVD(H, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_L)  { MOVD(V, L); MDECV; MOVD(L, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_A)  { MOVD(V, A); MDECV; MOVD(A, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_XH) { MOVD(V, XH); MDECV; MOVD(XH, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_XL) { MOVD(V, XL); MDECV; MOVD(XL, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_YH) { MOVD(V, YH); MDECV; MOVD(YH, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_YL) { MOVD(V, YL); MDECV; MOVD(YL, V); CLK(4, 1); OPEND; }
static OPFUNC(DEC_M)  { LEAHL; PUSH(ST); READ8; MDECV; POP(ST); WRITE8; CLK(11, 3); OPEND; }
static OPFUNC(DEC_MX) { LEAIX; PUSH(ST); READ8; MDECV; POP(ST); WRITE8; CLK(19, 4); OPEND; }
static OPFUNC(DEC_MY) { LEAIY; PUSH(ST); READ8; MDECV; POP(ST); WRITE8; CLK(19, 4); OPEND; }

// ---------------------------------------------------------------------------
//	スタック命令 -------------------------------------------------------------

static OPFUNC(PUSH_BC) { MOV16(UV, BC); MPUSHUV; CLK(11, 3); OPEND; }
static OPFUNC(PUSH_DE) { MOV16(UV, DE); MPUSHUV; CLK(11, 3); OPEND; }
static OPFUNC(PUSH_HL) { MOV16(UV, HL); MPUSHUV; CLK(11, 3); OPEND; }
static OPFUNC(PUSH_IX) { MOV16(UV, IX); MPUSHUV; CLK(11, 3); OPEND; }
static OPFUNC(PUSH_IY) { MOV16(UV, IY); MPUSHUV; CLK(11, 3); OPEND; }
static OPFUNC(PUSH_AF) { LOADUVAF; CLK(11, 3); MPUSHUV; OPEND; }

static OPFUNC(POP_BC) { MPOPUV; MOV16(BC, UV); CLK(10, 3); OPEND; }
static OPFUNC(POP_DE) { MPOPUV; MOV16(DE, UV); CLK(10, 3); OPEND; }
static OPFUNC(POP_HL) { MPOPUV; MOV16(HL, UV); CLK(10, 3); OPEND; }
static OPFUNC(POP_IX) { MPOPUV; MOV16(IX, UV); CLK(10, 3); OPEND; }
static OPFUNC(POP_IY) { MPOPUV; MOV16(IY, UV); CLK(10, 3); OPEND; }
static OPFUNC(POP_AF) { MPOPUV; CLK(10, 3); STOREAFUV; OPEND; }

// ---------------------------------------------------------------------------
//	16 ビットロード命令 ------------------------------------------------------

static OPFUNC(LD_BC_NN) { FETCH16; MOV16(BC, UV); CLK(10, 3); OPEND; }
static OPFUNC(LD_DE_NN) { FETCH16; MOV16(DE, UV); CLK(10, 3); OPEND; }
static OPFUNC(LD_HL_NN) { FETCH16; MOV16(HL, UV); CLK(10, 3); OPEND; }
static OPFUNC(LD_IX_NN) { FETCH16; MOV16(IX, UV); CLK(10, 3); OPEND; }
static OPFUNC(LD_IY_NN) { FETCH16; MOV16(IY, UV); CLK(10, 3); OPEND; }
static OPFUNC(LD_SP_NN) { FETCH16; MOV16(SP, UV); CLK(10, 3); OPEND; }

static OPFUNC(LD_MM_HL) { FETCH16; MOV16(ST, UV); CLK(16, 5); MOV16(UV, HL); WRITE16; OPEND; }
static OPFUNC(LD_MM_BC) { FETCH16; MOV16(ST, UV); CLK(20, 6); MOV16(UV, BC); WRITE16; OPEND; }
static OPFUNC(LD_MM_DE) { FETCH16; MOV16(ST, UV); CLK(20, 6); MOV16(UV, DE); WRITE16; OPEND; }
static OPFUNC(LD_MM_HL2){ FETCH16; MOV16(ST, UV); CLK(20, 6); MOV16(UV, HL); WRITE16; OPEND; }
static OPFUNC(LD_MM_SP) { FETCH16; MOV16(ST, UV); CLK(20, 6); MOV16(UV, SP); WRITE16; OPEND; }
static OPFUNC(LD_MM_IX) { FETCH16; MOV16(ST, UV); CLK(16, 5); MOV16(UV, IX); WRITE16; OPEND; }
static OPFUNC(LD_MM_IY) { FETCH16; MOV16(ST, UV); CLK(16, 5); MOV16(UV, IY); WRITE16; OPEND; }

static OPFUNC(LD_HL_MM) { FETCH16; MOV16(ST, UV); CLK(16, 5); READ16; MOV16(HL, UV); OPEND; }
static OPFUNC(LD_BC_MM) { FETCH16; MOV16(ST, UV); CLK(20, 6); READ16; MOV16(BC, UV); OPEND; }
static OPFUNC(LD_DE_MM) { FETCH16; MOV16(ST, UV); CLK(20, 6); READ16; MOV16(DE, UV); OPEND; }
static OPFUNC(LD_HL_MM2){ FETCH16; MOV16(ST, UV); CLK(20, 6); READ16; MOV16(HL, UV); OPEND; }
static OPFUNC(LD_SP_MM) { FETCH16; MOV16(ST, UV); CLK(20, 6); READ16; MOV16(SP, UV); OPEND; }
static OPFUNC(LD_IX_MM) { FETCH16; MOV16(ST, UV); CLK(16, 5); READ16; MOV16(IX, UV); OPEND; }
static OPFUNC(LD_IY_MM) { FETCH16; MOV16(ST, UV); CLK(16, 5); READ16; MOV16(IY, UV); OPEND; }

static OPFUNC(LD_SP_HL) { MOV16(UV, HL); CLK(6, 1); MOV16(SP, UV); OPEND; }
static OPFUNC(LD_SP_IX) { MOV16(UV, IX); CLK(6, 1); MOV16(SP, UV); OPEND; }
static OPFUNC(LD_SP_IY) { MOV16(UV, IY); CLK(6, 1); MOV16(SP, UV); OPEND; }

// ---------------------------------------------------------------------------
//	8 ビットロード命令 -------------------------------------------------------

static OPFUNC(LD_B_C)  { MOV(B, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_D)  { MOV(B, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_E)  { MOV(B, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_H)  { MOV(B, H); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_L)  { MOV(B, L); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_A)  { MOVD(B, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_XH) { MOV(B, XH); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_YH) { MOV(B, YH); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_XL) { MOV(B, XL); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_YL) { MOV(B, YL); CLK(4, 1); OPEND; }
static OPFUNC(LD_B_M)  { LEAHL; READ8; MOVD(B, V); CLK(7, 2); OPEND; }
static OPFUNC(LD_B_MX) { LEAIX; READ8; MOVD(B, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_B_MY) { LEAIY; READ8; MOVD(B, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_B_N)  { FETCH8; MOVD(B, V); CLK(7, 2); OPEND; FETCH8SUB; }

static OPFUNC(LD_C_B)  { MOV(C, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_D)  { MOV(C, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_E)  { MOV(C, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_H)  { MOV(C, H); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_L)  { MOV(C, L); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_A)  { MOVD(C, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_XH) { MOV(C, XH); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_YH) { MOV(C, YH); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_XL) { MOV(C, XL); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_YL) { MOV(C, YL); CLK(4, 1); OPEND; }
static OPFUNC(LD_C_M)  { LEAHL; READ8; MOVD(C, V); CLK(7, 2); OPEND; }
static OPFUNC(LD_C_MX) { LEAIX; READ8; MOVD(C, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_C_MY) { LEAIY; READ8; MOVD(C, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_C_N)  { FETCH8; MOVD(C, V); CLK(7, 2); OPEND; FETCH8SUB; }

static OPFUNC(LD_D_B)  { MOV(D, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_C)  { MOV(D, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_E)  { MOV(D, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_H)  { MOV(D, H); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_L)  { MOV(D, L); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_A)  { MOVD(D, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_XH) { MOV(D, XH); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_YH) { MOV(D, YH); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_XL) { MOV(D, XL); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_YL) { MOV(D, YL); CLK(4, 1); OPEND; }
static OPFUNC(LD_D_M)  { LEAHL; READ8; MOVD(D, V); CLK(7, 2); OPEND; }
static OPFUNC(LD_D_MX) { LEAIX; READ8; MOVD(D, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_D_MY) { LEAIY; READ8; MOVD(D, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_D_N)  { FETCH8; MOVD(D, V); CLK(7, 2); OPEND; FETCH8SUB; }

static OPFUNC(LD_E_B)  { MOV(E, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_C)  { MOV(E, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_D)  { MOV(E, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_H)  { MOV(E, H); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_L)  { MOV(E, L); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_A)  { MOVD(E, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_XH) { MOV(E, XH); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_YH) { MOV(E, YH); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_XL) { MOV(E, XL); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_YL) { MOV(E, YL); CLK(4, 1); OPEND; }
static OPFUNC(LD_E_M)  { LEAHL; READ8; MOVD(E, V); CLK(7, 2); OPEND; }
static OPFUNC(LD_E_MX) { LEAIX; READ8; MOVD(E, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_E_MY) { LEAIY; READ8; MOVD(E, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_E_N)  { FETCH8; MOVD(E, V); CLK(7, 2); OPEND; FETCH8SUB; }

static OPFUNC(LD_H_B)  { MOV(H, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_H_C)  { MOV(H, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_H_D)  { MOV(H, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_H_E)  { MOV(H, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_H_L)  { MOV(H, L); CLK(4, 1); OPEND; }
static OPFUNC(LD_H_A)  { MOVD(H, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_H_M)  { LEAHL; READ8; MOVD(H, V); CLK(7, 2); OPEND; }
static OPFUNC(LD_H_MX) { LEAIX; READ8; MOVD(H, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_H_MY) { LEAIY; READ8; MOVD(H, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_H_N)  { FETCH8; MOVD(H, V); CLK(7, 2); OPEND; FETCH8SUB; }

static OPFUNC(LD_L_B)  { MOV(L, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_L_C)  { MOV(L, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_L_D)  { MOV(L, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_L_E)  { MOV(L, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_L_H)  { MOV(L, H); CLK(4, 1); OPEND; }
static OPFUNC(LD_L_A)  { MOVD(L, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_L_M)  { LEAHL; READ8; MOVD(L, V); CLK(7, 2); OPEND; }
static OPFUNC(LD_L_MX) { LEAIX; READ8; MOVD(L, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_L_MY) { LEAIY; READ8; MOVD(L, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_L_N)  { FETCH8; MOVD(L, V); CLK(7, 2); OPEND; FETCH8SUB; }

static OPFUNC(LD_A_B)  { MOVD(A, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_C)  { MOVD(A, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_D)  { MOVD(A, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_E)  { MOVD(A, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_H)  { MOVD(A, H); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_L)  { MOVD(A, L); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_XH) { MOVD(A, XH); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_YH) { MOVD(A, YH); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_XL) { MOVD(A, XL); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_YL) { MOVD(A, YL); CLK(4, 1); OPEND; }
static OPFUNC(LD_A_M)  { LEAHL; READ8; MOVD(A, V); CLK(7, 2); OPEND; }
static OPFUNC(LD_A_MX) { LEAIX; READ8; MOVD(A, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_A_MY) { LEAIY; READ8; MOVD(A, V); CLK(15, 3); OPEND; }
static OPFUNC(LD_A_N)  { FETCH8; MOVD(A, V); CLK(7, 2); OPEND; FETCH8SUB; }

static OPFUNC(LD_XH_B)  { MOV(XH, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_XH_C)  { MOV(XH, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_XH_D)  { MOV(XH, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_XH_E)  { MOV(XH, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_XH_A)  { MOVD(XH, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_XH_XL) { MOV(XH, XL); CLK(4, 1); OPEND; }
static OPFUNC(LD_XH_N)  { FETCH8; MOVD(XH, V); CLK(7, 2); OPEND; FETCH8SUB; }

static OPFUNC(LD_YH_B)  { MOV(YH, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_YH_C)  { MOV(YH, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_YH_D)  { MOV(YH, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_YH_E)  { MOV(YH, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_YH_A)  { MOVD(YH, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_YH_YL) { MOV(YH, YL); CLK(4, 1); OPEND; }
static OPFUNC(LD_YH_N)  { FETCH8; MOVD(YH, V); CLK(4, 1); OPEND; FETCH8SUB; }

static OPFUNC(LD_XL_B)  { MOV(XL, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_XL_C)  { MOV(XL, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_XL_D)  { MOV(XL, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_XL_E)  { MOV(XL, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_XL_A)  { MOVD(XL, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_XL_XH) { MOV(XL, XH); CLK(4, 1); OPEND; }
static OPFUNC(LD_XL_N)  { FETCH8; MOVD(XL, V); CLK(7, 2); OPEND; FETCH8SUB; }

static OPFUNC(LD_YL_B)  { MOV(YL, B); CLK(4, 1); OPEND; }
static OPFUNC(LD_YL_C)  { MOV(YL, C); CLK(4, 1); OPEND; }
static OPFUNC(LD_YL_D)  { MOV(YL, D); CLK(4, 1); OPEND; }
static OPFUNC(LD_YL_E)  { MOV(YL, E); CLK(4, 1); OPEND; }
static OPFUNC(LD_YL_A)  { MOVD(YL, A); CLK(4, 1); OPEND; }
static OPFUNC(LD_YL_YH) { MOV(YL, YH); CLK(4, 1); OPEND; }
static OPFUNC(LD_YL_N)  { FETCH8; MOVD(YL, V); CLK(4, 1); OPEND; FETCH8SUB; }

static OPFUNC(LD_M_B)	{ LEAHL; MOVD(V, B); WRITE8; CLK(7, 2); OPEND; }
static OPFUNC(LD_M_C)	{ LEAHL; MOVD(V, C); WRITE8; CLK(7, 2); OPEND; }
static OPFUNC(LD_M_D)	{ LEAHL; MOVD(V, D); WRITE8; CLK(7, 2); OPEND; }
static OPFUNC(LD_M_E)	{ LEAHL; MOVD(V, E); WRITE8; CLK(7, 2); OPEND; }
static OPFUNC(LD_M_H)	{ LEAHL; MOVD(V, H); WRITE8; CLK(7, 2); OPEND; }
static OPFUNC(LD_M_L)	{ LEAHL; MOVD(V, L); WRITE8; CLK(7, 2); OPEND; }
static OPFUNC(LD_M_A)	{ LEAHL; MOVD(V, A); WRITE8; CLK(7, 2); OPEND; }
static OPFUNC(LD_M_N)	{ FETCH8; LEAHL; CLK(10, 3); WRITE8;  OPEND; FETCH8SUB; }

static OPFUNC(LD_MX_B)  { LEAIX; MOVD(V, B); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MX_C)  { LEAIX; MOVD(V, C); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MX_D)  { LEAIX; MOVD(V, D); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MX_E)  { LEAIX; MOVD(V, E); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MX_H)  { LEAIX; MOVD(V, H); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MX_L)  { LEAIX; MOVD(V, L); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MX_A)  { LEAIX; MOVD(V, A); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MX_N)  { FETCH16; MOVSX(ST, V); ADD16(ST, IX); CLK(19, 3); MOVD(V, U); WRITE8; OPEND; }

static OPFUNC(LD_MY_B)  { LEAIY; MOVD(V, B); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MY_C)  { LEAIY; MOVD(V, C); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MY_D)  { LEAIY; MOVD(V, D); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MY_E)  { LEAIY; MOVD(V, E); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MY_H)  { LEAIY; MOVD(V, H); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MY_L)  { LEAIY; MOVD(V, L); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MY_A)  { LEAIY; MOVD(V, A); WRITE8; CLK(19, 3); OPEND; }
static OPFUNC(LD_MY_N)  { FETCH16; MOVSX(ST, V); ADD16(ST, IY); CLK(19, 3); MOVD(V, U); WRITE8; OPEND; }

static OPFUNC(LD_BC_A) { MOV16(ST, BC); MOVD(V, A); CLK(7, 2); WRITE8; OPEND; }
static OPFUNC(LD_DE_A) { MOV16(ST, DE); MOVD(V, A); CLK(7, 2); WRITE8; OPEND; }
static OPFUNC(LD_MM_A) { FETCH16; MOV16(ST, UV); MOVD(V, A); CLK(13,  3); WRITE8; OPEND; }
static OPFUNC(LD_A_BC) { MOV16(ST, BC); READ8; MOVD(A, V); CLK(7, 2); OPEND; }
static OPFUNC(LD_A_DE) { MOV16(ST, DE); READ8; MOVD(A, V); CLK(7, 2); OPEND; }
static OPFUNC(LD_A_MM) { FETCH16; MOV16(ST, UV); READ8; CLK(13, 3); MOVD(A, V); OPEND; }

static OPFUNC(LD_A_I) { MOVD(A, I); TESTIFF; CLK(9, 2); OPEND; }
static OPFUNC(LD_A_R) { LOADAR; TESTIFF; CLK(9, 2); OPEND; }
static OPFUNC(LD_I_A) { MOVD(I, A); CLK(9, 2); OPEND; }
static OPFUNC(LD_R_A) { STORERA; CLK(9, 2); OPEND; }

// ---------------------------------------------------------------------------
//	ブロック入出力命令 -------------------------------------------------------

static OPFUNC(INI) { MINX; INC16(HL); CLK(16, 3); OPEND; inp_sync: PCDEC2; OPEND; }
static OPFUNC(IND) { MINX; DEC16(HL); CLK(16, 3); OPEND; inp_sync: PCDEC2; OPEND; }
static OPFUNC(OUTI) { MOUTX; INC16(HL); CLK(16, 3); OPEND; outx_sync: PCDEC2; OPEND; }
static OPFUNC(OUTD) { MOUTX; DEC16(HL); CLK(16, 3); OPEND; outx_sync: PCDEC2; OPEND; }

static OPFUNC(INIR) { MINX;  INC16(HL); MIFZ(END); CLK(20, 3); inp_sync: PCDEC2; OPEND; END: CLK(16, 3); OPEND; }
static OPFUNC(INDR) { MINX;  DEC16(HL); MIFZ(END); CLK(20, 3); inp_sync: PCDEC2; OPEND; END: CLK(16, 3); OPEND; }
static OPFUNC(OTIR) { MOUTX; INC16(HL); MIFZ(END); CLK(20, 3); outx_sync: PCDEC2; OPEND; END: CLK(16, 3); OPEND; }
static OPFUNC(OTDR) { MOUTX; DEC16(HL); MIFZ(END); CLK(20, 3); outx_sync: PCDEC2; OPEND; END: CLK(16, 3); OPEND; }

// ---------------------------------------------------------------------------
//	ブロック転送命令 ---------------------------------------------------------

static OPFUNC(LDI) { MLDI; CLK(16, 4); OPEND; }
static OPFUNC(LDD) { MLDD; CLK(16, 4); OPEND; }

static OPFUNC(LDIR) { MLDI; MIFPO(END); CLK(21, 4); PCDEC2; OPEND; END: CLK(16, 4); OPEND; }
static OPFUNC(LDDR) { MLDD; MIFPO(END); CLK(21, 4); PCDEC2; OPEND; END: CLK(16, 4); OPEND; }

// ---------------------------------------------------------------------------
//	ブロックサーチ命令 -------------------------------------------------------

static OPFUNC(CPI) { MCPI; CLK(16, 4); OPEND; }
static OPFUNC(CPD) { MCPD; CLK(16, 4); OPEND; }

static OPFUNC(CPIR) { MCPI; MIFZ(END); MIFPO(END); CLK(20, 4); PCDEC2; OPEND; END: CLK(16, 4); OPEND; }
static OPFUNC(CPDR) { MCPD; MIFZ(END); MIFPO(END); CLK(20, 4); PCDEC2; OPEND; END: CLK(16, 4); OPEND; }

// ---------------------------------------------------------------------------
//	CB 関係 ------------------------------------------------------------------

#define MBITV(n)		\
 /* 書き込み省略 */		__asm { pop PQ } \
						__asm { test V,n } \
						__asm { setz U } \
						CLRF(SF+NF+ZF) \
						__asm { shl U,6 } \
						SETF(HF) \
						__asm { and V,n } \
						__asm { mov FLAGX,V } \
						__asm { and V,80h } \
						__asm { or ah,U } \
						__asm { shl U, 1 } \
						__asm { not U } \
						__asm { or ah,V } \
						__asm { and ah,U }

#define MRESV(n)		__asm { and V,not n }

#define MSETV(n)		__asm { or V,n }

static OPFUNC(RLCV)	{ MRLCV; OPEND; }	static OPFUNC(RLV)	{ MRLV;  OPEND; }
static OPFUNC(RRCV)	{ MRRCV; OPEND; }	static OPFUNC(RRV)	{ MRRV;  OPEND; }
static OPFUNC(SLAV)	{ MSLAV; OPEND; }	static OPFUNC(SRAV)	{ MSRAV; OPEND; }
static OPFUNC(SLLV)	{ MSLLV; OPEND; }	static OPFUNC(SRLV)	{ MSRLV; OPEND; }

static OPFUNC(BITV_0)	{ MBITV(0x01); OPEND; }	static OPFUNC(BITV_1)	{ MBITV(0x02); OPEND; }
static OPFUNC(BITV_2)	{ MBITV(0x04); OPEND; }	static OPFUNC(BITV_3)	{ MBITV(0x08); OPEND; }
static OPFUNC(BITV_4)	{ MBITV(0x10); OPEND; }	static OPFUNC(BITV_5)	{ MBITV(0x20); OPEND; }
static OPFUNC(BITV_6)	{ MBITV(0x40); OPEND; }	static OPFUNC(BITV_7)	{ MBITV(0x80); OPEND; }

static OPFUNC(RESV_0)	{ MRESV(0x01); OPEND; }	static OPFUNC(RESV_1)	{ MRESV(0x02); OPEND; }
static OPFUNC(RESV_2)	{ MRESV(0x04); OPEND; }	static OPFUNC(RESV_3)	{ MRESV(0x08); OPEND; }
static OPFUNC(RESV_4)	{ MRESV(0x10); OPEND; }	static OPFUNC(RESV_5)	{ MRESV(0x20); OPEND; }
static OPFUNC(RESV_6)	{ MRESV(0x40); OPEND; }	static OPFUNC(RESV_7)	{ MRESV(0x80); OPEND; }

static OPFUNC(SETV_0)	{ MSETV(0x01); OPEND; }	static OPFUNC(SETV_1)	{ MSETV(0x02); OPEND; }
static OPFUNC(SETV_2)	{ MSETV(0x04); OPEND; }	static OPFUNC(SETV_3)	{ MSETV(0x08); OPEND; }
static OPFUNC(SETV_4)	{ MSETV(0x10); OPEND; }	static OPFUNC(SETV_5)	{ MSETV(0x20); OPEND; }
static OPFUNC(SETV_6)	{ MSETV(0x40); OPEND; }	static OPFUNC(SETV_7)	{ MSETV(0x80); OPEND; }

static const OpFuncPtr OpTableCB2[0x20] =
{
O_RLCV,		O_RRCV,		O_RLV,		O_RRV,		O_SLAV,		O_SRAV,		O_SLLV,		O_SRLV,
O_BITV_0,	O_BITV_1,	O_BITV_2,	O_BITV_3,	O_BITV_4,	O_BITV_5,	O_BITV_6,	O_BITV_7,	
O_RESV_0,	O_RESV_1,	O_RESV_2,	O_RESV_3,	O_RESV_4,	O_RESV_5,	O_RESV_6,	O_RESV_7,	
O_SETV_0,	O_SETV_1,	O_SETV_2,	O_SETV_3,	O_SETV_4,	O_SETV_5,	O_SETV_6,	O_SETV_7,	
};

static OPFUNC(CB_B)	{ CLK(8, 2); MOVD(V,B); __asm { call OpTableCB2[PQ*4] } MOVD(B, V); OPEND; }
static OPFUNC(CB_C)	{ CLK(8, 2); MOVD(V,C); __asm { call OpTableCB2[PQ*4] } MOVD(C, V); OPEND; }
static OPFUNC(CB_D)	{ CLK(8, 2); MOVD(V,D); __asm { call OpTableCB2[PQ*4] } MOVD(D, V); OPEND; }
static OPFUNC(CB_E)	{ CLK(8, 2); MOVD(V,E); __asm { call OpTableCB2[PQ*4] } MOVD(E, V); OPEND; }
static OPFUNC(CB_H)	{ CLK(8, 2); MOVD(V,H); __asm { call OpTableCB2[PQ*4] } MOVD(H, V); OPEND; }
static OPFUNC(CB_L)	{ CLK(8, 2); MOVD(V,L); __asm { call OpTableCB2[PQ*4] } MOVD(L, V); OPEND; }
static OPFUNC(CB_A)	{ CLK(8, 2); MOVD(V,A); __asm { call OpTableCB2[PQ*4] } MOVD(A, V); OPEND; }

static OPFUNC(CB_M)
{
	CLK(12, 3); PUSH(PQ); PUSH(ST); READ8; POP(ST); POP(PQ); 
	__asm { call OpTableCB2[PQ*4] } CLK(3, 1); WRITE8; OPEND;
}

static OPFUNC(CB_MB)
{
	CLK(12, 3); PUSH(PQ); PUSH(ST); READ8; POP(ST); POP(PQ); 
	__asm { call OpTableCB2[PQ*4] } MOVD(B, V); CLK(3, 1); WRITE8; OPEND;
}

static OPFUNC(CB_MC)
{
	CLK(12, 3); PUSH(PQ); PUSH(ST); READ8; POP(ST); POP(PQ); 
	__asm { call OpTableCB2[PQ*4] } MOVD(C, V); CLK(3, 1); WRITE8; OPEND;
}

static OPFUNC(CB_MD)
{
	CLK(12, 3); PUSH(PQ); PUSH(ST); READ8; POP(ST); POP(PQ); 
	__asm { call OpTableCB2[PQ*4] } MOVD(D, V); CLK(3, 1); WRITE8; OPEND;
}

static OPFUNC(CB_ME)
{
	CLK(12, 3); PUSH(PQ); PUSH(ST); READ8; POP(ST); POP(PQ); 
	__asm { call OpTableCB2[PQ*4] } MOVD(E, V); CLK(3, 1); WRITE8; OPEND;
}

static OPFUNC(CB_MH)
{
	CLK(12, 3); PUSH(PQ); PUSH(ST); READ8; POP(ST); POP(PQ); 
	__asm { call OpTableCB2[PQ*4] } MOVD(H, V); CLK(3, 1); WRITE8; OPEND;
}

static OPFUNC(CB_ML)
{
	CLK(12, 3); PUSH(PQ); PUSH(ST); READ8; POP(ST); POP(PQ); 
	__asm { call OpTableCB2[PQ*4] } MOVD(L, V); CLK(3, 1); WRITE8; OPEND;
}

static OPFUNC(CB_MA)
{
	CLK(12, 3); PUSH(PQ); PUSH(ST); READ8; POP(ST); POP(PQ); 
	__asm { call OpTableCB2[PQ*4] } MOVD(A, V); CLK(3, 1); WRITE8; OPEND;
}

// ---------------------------------------------------------------------------
//	連続 DD/FD prefix の時のための処理ブロック -------------------------------

static OPFUNC(DEC_PC)
{
	__asm { sub eax,1000000h };
	CLK(4, 1); 
	PCDEC1
	OPEND;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

void O_CODE_ED();
void O_CODE_DD();
void O_CODE_FD();
void O_CODE_CB();
void O_CODE_DDCB();
void O_CODE_FDCB();

void O_DI();
void O_EI();

static const OpFuncPtr OpTable[0x100] =
{
O_NOP,		O_LD_BC_NN,	O_LD_BC_A,	O_INC_BC,	O_INC_B,	O_DEC_B,	O_LD_B_N,	O_RLCA,		
O_EX_AF_AF,	O_ADDHL_BC,	O_LD_A_BC,	O_DEC_BC,	O_INC_C,	O_DEC_C,	O_LD_C_N,	O_RRCA,		
O_DJNZ,		O_LD_DE_NN,	O_LD_DE_A,	O_INC_DE,	O_INC_D,	O_DEC_D,	O_LD_D_N,	O_RLA,		
O_JR,		O_ADDHL_DE,	O_LD_A_DE,	O_DEC_DE,	O_INC_E,	O_DEC_E,	O_LD_E_N,	O_RRA,		
O_JR_NZ,	O_LD_HL_NN,	O_LD_MM_HL,	O_INC_HL,	O_INC_H,	O_DEC_H,	O_LD_H_N,	O_DAA,		
O_JR_Z,		O_ADDHL_HL,	O_LD_HL_MM,	O_DEC_HL,	O_INC_L,	O_DEC_L,	O_LD_L_N,	O_CPL,		
O_JR_NC,	O_LD_SP_NN,	O_LD_MM_A,	O_INC_SP,	O_INC_M,	O_DEC_M,	O_LD_M_N,	O_SCF,		
O_JR_C,		O_ADDHL_SP,	O_LD_A_MM,	O_DEC_SP,	O_INC_A,	O_DEC_A,	O_LD_A_N,	O_CCF,		

O_NOP,		O_LD_B_C,	O_LD_B_D,	O_LD_B_E,	O_LD_B_H,	O_LD_B_L,	O_LD_B_M,	O_LD_B_A,	
O_LD_C_B,	O_NOP,		O_LD_C_D,	O_LD_C_E,	O_LD_C_H,	O_LD_C_L,	O_LD_C_M,	O_LD_C_A,	
O_LD_D_B,	O_LD_D_C,	O_NOP,		O_LD_D_E,	O_LD_D_H,	O_LD_D_L,	O_LD_D_M,	O_LD_D_A,	
O_LD_E_B,	O_LD_E_C,	O_LD_E_D,	O_NOP,		O_LD_E_H,	O_LD_E_L,	O_LD_E_M,	O_LD_E_A,	
O_LD_H_B,	O_LD_H_C,	O_LD_H_D,	O_LD_H_E,	O_NOP,		O_LD_H_L,	O_LD_H_M,	O_LD_H_A,	
O_LD_L_B,	O_LD_L_C,	O_LD_L_D,	O_LD_L_E,	O_LD_L_H,	O_NOP,		O_LD_L_M,	O_LD_L_A,	
O_LD_M_B,	O_LD_M_C,	O_LD_M_D,	O_LD_M_E,	O_LD_M_H,	O_LD_M_L,	O_HALT,		O_LD_M_A,	
O_LD_A_B,	O_LD_A_C,	O_LD_A_D,	O_LD_A_E,	O_LD_A_H,	O_LD_A_L,	O_LD_A_M,	O_NOP,		

O_ADD_A_B,	O_ADD_A_C,	O_ADD_A_D,	O_ADD_A_E,	O_ADD_A_H,	O_ADD_A_L,	O_ADD_A_M,	O_ADD_A_A,	
O_ADC_A_B,	O_ADC_A_C,	O_ADC_A_D,	O_ADC_A_E,	O_ADC_A_H,	O_ADC_A_L,	O_ADC_A_M,	O_ADC_A_A,	
O_SUB_B,	O_SUB_C,	O_SUB_D,	O_SUB_E,	O_SUB_H,	O_SUB_L,	O_SUB_M,	O_SUB_A,	
O_SBC_A_B,	O_SBC_A_C,	O_SBC_A_D,	O_SBC_A_E,	O_SBC_A_H,	O_SBC_A_L,	O_SBC_A_M,	O_SBC_A_A,	
O_AND_B,	O_AND_C,	O_AND_D,	O_AND_E,	O_AND_H,	O_AND_L,	O_AND_M,	O_AND_A,	
O_XOR_B,	O_XOR_C,	O_XOR_D,	O_XOR_E,	O_XOR_H,	O_XOR_L,	O_XOR_M,	O_XOR_A,	
O_OR_B,		O_OR_C,		O_OR_D,		O_OR_E,		O_OR_H,		O_OR_L,		O_OR_M,		O_OR_A,		
O_CP_B,		O_CP_C,		O_CP_D,		O_CP_E,		O_CP_H,		O_CP_L,		O_CP_M,		O_CP_A,		

O_RET_NZ,	O_POP_BC,	O_JP_NZ,	O_JP,		O_CALL_NZ,	O_PUSH_BC,	O_ADD_A_N,	O_RST00,	
O_RET_Z,	O_RET,		O_JP_Z,		O_CODE_CB,	O_CALL_Z,	O_CALL,		O_ADC_A_N,	O_RST08,	
O_RET_NC,	O_POP_DE,	O_JP_NC,	O_OUT_N_A,	O_CALL_NC,	O_PUSH_DE,	O_SUB_N,	O_RST10,	
O_RET_C,	O_EXX,		O_JP_C,		O_IN_A_N,	O_CALL_C,	O_CODE_DD,	O_SBC_A_N,	O_RST18,	
O_RET_PO,	O_POP_HL,	O_JP_PO,	O_EX_SP_HL,	O_CALL_PO,	O_PUSH_HL,	O_AND_N,	O_RST20,	
O_RET_PE,	O_JP_HL,	O_JP_PE,	O_EX_DE_HL,	O_CALL_PE,	O_CODE_ED,	O_XOR_N,	O_RST28,	
O_RET_P,	O_POP_AF,	O_JP_P,		O_DI,		O_CALL_P,	O_PUSH_AF,	O_OR_N,		O_RST30,	
O_RET_M,	O_LD_SP_HL,	O_JP_M,		O_EI,		O_CALL_M,	O_CODE_FD,	O_CP_N,		O_RST38,	
};

static const OpFuncPtr OpTableDD[0x100] =
{
O_NOP,		O_LD_BC_NN,	O_LD_BC_A,	O_INC_BC,	O_INC_B,	O_DEC_B,	O_LD_B_N,	O_RLCA,		
O_EX_AF_AF,	O_ADDIX_BC,	O_LD_A_BC,	O_DEC_BC,	O_INC_C,	O_DEC_C,	O_LD_C_N,	O_RRCA,		
O_DJNZ,		O_LD_DE_NN,	O_LD_DE_A,	O_INC_DE,	O_INC_D,	O_DEC_D,	O_LD_D_N,	O_RLA,		
O_JR,		O_ADDIX_DE,	O_LD_A_DE,	O_DEC_DE,	O_INC_E,	O_DEC_E,	O_LD_E_N,	O_RRA,		
O_JR_NZ,	O_LD_IX_NN,	O_LD_MM_IX,	O_INC_IX,	O_INC_XH,	O_DEC_XH,	O_LD_XH_N,	O_DAA,		
O_JR_Z,		O_ADDIX_IX,	O_LD_IX_MM,	O_DEC_IX,	O_INC_XL,	O_DEC_XL,	O_LD_XL_N,	O_CPL,		
O_JR_NC,	O_LD_SP_NN,	O_LD_MM_A,	O_INC_SP,	O_INC_MX,	O_DEC_MX,	O_LD_MX_N,	O_SCF,		
O_JR_C,		O_ADDIX_SP,	O_LD_A_MM,	O_DEC_SP,	O_INC_A,	O_DEC_A,	O_LD_A_N,	O_CCF,		

O_NOP,		O_LD_B_C,	O_LD_B_D,	O_LD_B_E,	O_LD_B_XH,	O_LD_B_XL,	O_LD_B_MX,	O_LD_B_A,	
O_LD_C_B,	O_NOP,		O_LD_C_D,	O_LD_C_E,	O_LD_C_XH,	O_LD_C_XL,	O_LD_C_MX,	O_LD_C_A,	
O_LD_D_B,	O_LD_D_C,	O_NOP,		O_LD_D_E,	O_LD_D_XH,	O_LD_D_XL,	O_LD_D_MX,	O_LD_D_A,	
O_LD_E_B,	O_LD_E_C,	O_LD_E_D,	O_NOP,		O_LD_E_XH,	O_LD_E_XL,	O_LD_E_MX,	O_LD_E_A,	
O_LD_XH_B,	O_LD_XH_C,	O_LD_XH_D,	O_LD_XH_E,	O_NOP,		O_LD_XH_XL,	O_LD_H_MX,	O_LD_XH_A,	
O_LD_XL_B,	O_LD_XL_C,	O_LD_XL_D,	O_LD_XL_E,	O_LD_XL_XH,	O_NOP,		O_LD_L_MX,	O_LD_XL_A,	
O_LD_MX_B,	O_LD_MX_C,	O_LD_MX_D,	O_LD_MX_E,	O_LD_MX_H,	O_LD_MX_L,	O_HALT,		O_LD_MX_A,	
O_LD_A_B,	O_LD_A_C,	O_LD_A_D,	O_LD_A_E,	O_LD_A_XH,	O_LD_A_XL,	O_LD_A_MX,	O_NOP,		

O_ADD_A_B,	O_ADD_A_C,	O_ADD_A_D,	O_ADD_A_E,	O_ADD_A_XH,	O_ADD_A_XL,	O_ADD_A_MX,	O_ADD_A_A,	
O_ADC_A_B,	O_ADC_A_C,	O_ADC_A_D,	O_ADC_A_E,	O_ADC_A_XH,	O_ADC_A_XL,	O_ADC_A_MX,	O_ADC_A_A,	
O_SUB_B,	O_SUB_C,	O_SUB_D,	O_SUB_E,	O_SUB_XH,	O_SUB_XL,	O_SUB_MX,	O_SUB_A,	
O_SBC_A_B,	O_SBC_A_C,	O_SBC_A_D,	O_SBC_A_E,	O_SBC_A_XH,	O_SBC_A_XL,	O_SBC_A_MX,	O_SBC_A_A,	
O_AND_B,	O_AND_C,	O_AND_D,	O_AND_E,	O_AND_XH,	O_AND_XL,	O_AND_MX,	O_AND_A,	
O_XOR_B,	O_XOR_C,	O_XOR_D,	O_XOR_E,	O_XOR_XH,	O_XOR_XL,	O_XOR_MX,	O_XOR_A,	
O_OR_B,		O_OR_C,		O_OR_D,		O_OR_E,		O_OR_XH,	O_OR_XL,	O_OR_MX,	O_OR_A,		
O_CP_B,		O_CP_C,		O_CP_D,		O_CP_E,		O_CP_XH,	O_CP_XL,	O_CP_MX,	O_CP_A,		

O_RET_NZ,	O_POP_BC,	O_JP_NZ,	O_JP,		O_CALL_NZ,	O_PUSH_BC,	O_ADD_A_N,	O_RST00,	
O_RET_Z,	O_RET,		O_JP_Z,		O_CODE_DDCB,O_CALL_Z,	O_CALL,		O_ADC_A_N,	O_RST08,	
O_RET_NC,	O_POP_DE,	O_JP_NC,	O_OUT_N_A,	O_CALL_NC,	O_PUSH_DE,	O_SUB_N,	O_RST10,	
O_RET_C,	O_EXX,		O_JP_C,		O_IN_A_N,	O_CALL_C,	O_CODE_DD,	O_SBC_A_N,	O_RST18,	
O_RET_PO,	O_POP_IX,	O_JP_PO,	O_EX_SP_IX,	O_CALL_PO,	O_PUSH_IX,	O_AND_N,	O_RST20,	
O_RET_PE,	O_JP_IX,	O_JP_PE,	O_EX_DE_HL,	O_CALL_PE,	O_DEC_PC,	O_XOR_N,	O_RST28,	
O_RET_P,	O_POP_AF,	O_JP_P,		O_DI,		O_CALL_P,	O_PUSH_AF,	O_OR_N,		O_RST30,	
O_RET_M,	O_LD_SP_IX,	O_JP_M,		O_EI,		O_CALL_M,	O_DEC_PC,	O_CP_N,		O_RST38,	
};

static const OpFuncPtr OpTableFD[0x100] =
{
O_NOP,		O_LD_BC_NN,	O_LD_BC_A,	O_INC_BC,	O_INC_B,	O_DEC_B,	O_LD_B_N,	O_RLCA,		
O_EX_AF_AF,	O_ADDIY_BC,	O_LD_A_BC,	O_DEC_BC,	O_INC_C,	O_DEC_C,	O_LD_C_N,	O_RRCA,		
O_DJNZ,		O_LD_DE_NN,	O_LD_DE_A,	O_INC_DE,	O_INC_D,	O_DEC_D,	O_LD_D_N,	O_RLA,		
O_JR,		O_ADDIY_DE,	O_LD_A_DE,	O_DEC_DE,	O_INC_E,	O_DEC_E,	O_LD_E_N,	O_RRA,		
O_JR_NZ,	O_LD_IY_NN,	O_LD_MM_IY,	O_INC_IY,	O_INC_YH,	O_DEC_YH,	O_LD_YH_N,	O_DAA,		
O_JR_Z,		O_ADDIY_IY,	O_LD_IY_MM,	O_DEC_IY,	O_INC_YL,	O_DEC_YL,	O_LD_YL_N,	O_CPL,		
O_JR_NC,	O_LD_SP_NN,	O_LD_MM_A,	O_INC_SP,	O_INC_MY,	O_DEC_MY,	O_LD_MY_N,	O_SCF,		
O_JR_C,		O_ADDIY_SP,	O_LD_A_MM,	O_DEC_SP,	O_INC_A,	O_DEC_A,	O_LD_A_N,	O_CCF,		

O_NOP,		O_LD_B_C,	O_LD_B_D,	O_LD_B_E,	O_LD_B_YH,	O_LD_B_YL,	O_LD_B_MY,	O_LD_B_A,	
O_LD_C_B,	O_NOP,		O_LD_C_D,	O_LD_C_E,	O_LD_C_YH,	O_LD_C_YL,	O_LD_C_MY,	O_LD_C_A,	
O_LD_D_B,	O_LD_D_C,	O_NOP,		O_LD_D_E,	O_LD_D_YH,	O_LD_D_YL,	O_LD_D_MY,	O_LD_D_A,	
O_LD_E_B,	O_LD_E_C,	O_LD_E_D,	O_NOP,		O_LD_E_YH,	O_LD_E_YL,	O_LD_E_MY,	O_LD_E_A,	
O_LD_YH_B,	O_LD_YH_C,	O_LD_YH_D,	O_LD_YH_E,	O_NOP,		O_LD_YH_YL,	O_LD_H_MY,	O_LD_YH_A,	
O_LD_YL_B,	O_LD_YL_C,	O_LD_YL_D,	O_LD_YL_E,	O_LD_YL_YH,	O_NOP,		O_LD_L_MY,	O_LD_YL_A,	
O_LD_MY_B,	O_LD_MY_C,	O_LD_MY_D,	O_LD_MY_E,	O_LD_MY_H,	O_LD_MY_L,	O_HALT,		O_LD_MY_A,	
O_LD_A_B,	O_LD_A_C,	O_LD_A_D,	O_LD_A_E,	O_LD_A_YH,	O_LD_A_YL,	O_LD_A_MY,	O_NOP,		

O_ADD_A_B,	O_ADD_A_C,	O_ADD_A_D,	O_ADD_A_E,	O_ADD_A_YH,	O_ADD_A_YL,	O_ADD_A_MY,	O_ADD_A_A,	
O_ADC_A_B,	O_ADC_A_C,	O_ADC_A_D,	O_ADC_A_E,	O_ADC_A_YH,	O_ADC_A_YL,	O_ADC_A_MY,	O_ADC_A_A,	
O_SUB_B,	O_SUB_C,	O_SUB_D,	O_SUB_E,	O_SUB_YH,	O_SUB_YL,	O_SUB_MY,	O_SUB_A,	
O_SBC_A_B,	O_SBC_A_C,	O_SBC_A_D,	O_SBC_A_E,	O_SBC_A_YH,	O_SBC_A_YL,	O_SBC_A_MY,	O_SBC_A_A,	
O_AND_B,	O_AND_C,	O_AND_D,	O_AND_E,	O_AND_YH,	O_AND_YL,	O_AND_MY,	O_AND_A,	
O_XOR_B,	O_XOR_C,	O_XOR_D,	O_XOR_E,	O_XOR_YH,	O_XOR_YL,	O_XOR_MY,	O_XOR_A,	
O_OR_B,		O_OR_C,		O_OR_D,		O_OR_E,		O_OR_YH,	O_OR_YL,	O_OR_MY,	O_OR_A,		
O_CP_B,		O_CP_C,		O_CP_D,		O_CP_E,		O_CP_YH,	O_CP_YL,	O_CP_MY,	O_CP_A,		

O_RET_NZ,	O_POP_BC,	O_JP_NZ,	O_JP,		O_CALL_NZ,	O_PUSH_BC,	O_ADD_A_N,	O_RST00,	
O_RET_Z,	O_RET,		O_JP_Z,		O_CODE_FDCB,O_CALL_Z,	O_CALL,		O_ADC_A_N,	O_RST08,	
O_RET_NC,	O_POP_DE,	O_JP_NC,	O_OUT_N_A,	O_CALL_NC,	O_PUSH_DE,	O_SUB_N,	O_RST10,	
O_RET_C,	O_EXX,		O_JP_C,		O_IN_A_N,	O_CALL_C,	O_CODE_DD,	O_SBC_A_N,	O_RST18,	
O_RET_PO,	O_POP_IY,	O_JP_PO,	O_EX_SP_IY,	O_CALL_PO,	O_PUSH_IY,	O_AND_N,	O_RST20,	
O_RET_PE,	O_JP_IY,	O_JP_PE,	O_EX_DE_HL,	O_CALL_PE,	O_DEC_PC,	O_XOR_N,	O_RST28,	
O_RET_P,	O_POP_AF,	O_JP_P,		O_DI,		O_CALL_P,	O_PUSH_AF,	O_OR_N,		O_RST30,	
O_RET_M,	O_LD_SP_IY,	O_JP_M,		O_EI,		O_CALL_M,	O_DEC_PC,	O_CP_N,		O_RST38,	
};

static const OpFuncPtr OpTableED[0x80] =
{
O_IN_B_C,	O_OUT_C_B,	O_SBCHL_BC,	O_LD_MM_BC,	O_NEG,		O_RETN,		O_IM0,		O_LD_I_A,	
O_IN_C_C,	O_OUT_C_C,	O_ADCHL_BC,	O_LD_BC_MM,	O_NEG,		O_RETI,		O_IM0,		O_LD_R_A,	
O_IN_D_C,	O_OUT_C_D,	O_SBCHL_DE,	O_LD_MM_DE,	O_NEG,		O_RETN,		O_IM1,		O_LD_A_I,	
O_IN_E_C,	O_OUT_C_E,	O_ADCHL_DE,	O_LD_DE_MM,	O_NEG,		O_RETN,		O_IM2,		O_LD_A_R,	
O_IN_H_C,	O_OUT_C_H,	O_SBCHL_HL,	O_LD_MM_HL2,O_NEG,		O_RETN,		O_IM0,		O_RRD,		
O_IN_L_C,	O_OUT_C_L,	O_ADCHL_HL,	O_LD_HL_MM2,O_NEG,		O_RETN,		O_IM0,		O_RLD,		
O_IN_F_C,	O_OUT_C_Z,	O_SBCHL_SP,	O_LD_MM_SP,	O_NEG,		O_RETN,		O_IM1,		O_NOP,		
O_IN_A_C,	O_OUT_C_A,	O_ADCHL_SP,	O_LD_SP_MM,	O_NEG,		O_RETN,		O_IM2,		O_NOP,		

O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		
O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		
O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		
O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		
O_LDI,		O_CPI,		O_INI,		O_OUTI,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		
O_LDD,		O_CPD,		O_IND,		O_OUTD,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		
O_LDIR,		O_CPIR,		O_INIR,		O_OTIR,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		
O_LDDR,		O_CPDR,		O_INDR,		O_OTDR,		O_NOP,		O_NOP,		O_NOP,		O_NOP,		
};

static const OpFuncPtr OpTableCB1[8] =
{
	O_CB_B, O_CB_C, O_CB_D, O_CB_E, O_CB_H, O_CB_L, O_CB_M, O_CB_A,		
};

static const OpFuncPtr OpTableCB1X[8] =
{
	O_CB_MB, O_CB_MC, O_CB_MD, O_CB_ME, O_CB_MH, O_CB_ML, O_CB_M, O_CB_MA,
};

// ---------------------------------------------------------------------------
//  prefix 付命令処理 --------------------------------------------------------

static OPFUNC(CODE_DD)
{
	FETCH8;
	__asm { add eax,01000000h }
	CLK(4, 1);
	__asm { jmp OpTableDD[UV*4] }
	FETCH8SUB; 
}

static OPFUNC(CODE_FD)
{
	FETCH8;
	__asm { add eax,01000000h }
	CLK(4, 1);
	__asm { jmp OpTableFD[UV*4] }
	FETCH8SUB; 
}

static OPFUNC(CODE_ED)
{
	__asm { mov PQ,INSTWAIT }
	__asm { sub PQ,CLOCKCOUNT }		// ST = -(prevcount)
	FETCH8;
	__asm { sub UV,40h } 
	__asm { add eax,01000000h }
	__asm { cmp UV,80h }
	__asm { jnb none }
	__asm { jmp OpTableED[UV*4] }
none:
	CLK(8, 2);
	OPEND;
	FETCH8SUB; 
}

static OPFUNC(CODE_CB)
{
	FETCH8
	__asm { mov ST,HL }
	
	__asm { mov PQ,UV }
	__asm { add eax,01000000h }
	__asm { shr PQ,3 }
	__asm { and UV,7 } 
	__asm { and PQ,31 } 
	__asm { jmp OpTableCB1[UV*4] }
	
	FETCH8SUB; 
}

static OPFUNC(CODE_DDCB)
{
	LEAIX;					// ST <- IX+d 
	__asm { push ST }
	FETCH8
	__asm { pop ST }
	
	CLK(4, 2);
	__asm { mov PQ,UV }
	__asm { shr PQ,3 }
	__asm { and UV,7 } 
	__asm { and PQ,31 } 
	__asm { jmp OpTableCB1X[UV*4] }
	
	FETCH8SUB; 
}

static OPFUNC(CODE_FDCB)
{
	LEAIY;					// ST <- IY+d
	__asm { push ST }
	FETCH8
	__asm { pop ST }
	
	CLK(4, 2);
	__asm { mov PQ,UV }
	__asm { shr PQ,3 }
	__asm { and UV,7 } 
	__asm { and PQ,31 } 
	__asm { jmp OpTableCB1X[UV*4] }
	
	FETCH8SUB; 
}

// ---------------------------------------------------------------------------
//	割り込む -----------------------------------------------------------------

static OPFUNC(INTR)
{
	__asm
	{
		mov dl,IFF1
		and dl,CPU.intr
		jnz process
		ret
	}
process:
	__asm
	{
		test WAITSTATE, 1
		jz nohalt
		PCINC1
	nohalt:
		mov WAITSTATE, 0
		
		add eax,1000000h		// R++
		xor ecx,ecx
		mov IFF1,cl
		mov UV, CPU.intack
		call Bus_In
		// ST = INTNO
		movzx ebx,INTMODE
		cmp ebx,1
		jc int0
		jz int1

// int2:
		movzx edx,I
		shl edx,8
		add ecx,edx
	}
	READ16;
	PUSH(UV); 
	GETPC(UV); 
	MPUSHUV; 
	POP(INST); 
	MJUMP
	CLK(19, 4);							// ベクタ読み込み + push ? 
	__asm { stc }
	__asm { ret }

int1:
	GETPC(UV);
	MPUSHUV;
	SETPC(0x38);
	CLK(13, 2);								// push pc ?
	__asm { stc }
	__asm { ret }

int0:
	// 命令処理しないと
	CLK(13, 2);								// nop ?
	__asm { stc }
	__asm { ret }
}

// ---------------------------------------------------------------------------
// 命令処理用マクロ

#if FASTFETCH
	#if 1
		#define FETCHCODE(n)	__asm { cmp INST,INSTLIM } \
								__asm { jnb indirect_##n } \
								__asm { movzx edx,byte ptr [INST] } \
								__asm { mov ecx,INSTWAIT } \
								__asm { inc INST } \
								__asm { add CLOCKCOUNT,ecx } \
								fetchend_##n: \
								__asm { add eax,1000000h } \
								__asm { call OpTable[edx*4] } 

		#define FETCHSUB(n)		indirect_##n: \
								__asm { call Fetcher8 } \
								__asm { jmp fetchend_##n }
	#else
		#define FETCHCODE(n)	__asm { call Fetcher8 } \
								__asm { add eax,1000000h } \
								__asm { call OpTable[edx*4] } 

		#define FETCHSUB(n)		
	#endif
#else
	#define FETCHCODE(n)	__asm { mov ecx, edi } \
							__asm { inc edi } \
							__asm { call Reader8 } \
							__asm { add eax,1000000h } \
							__asm { call OpTable[edx*4] } 
	#define FETCHSUB(n)
#endif



// ---------------------------------------------------------------------------
//	命令処理
//	前提条件: thiscall
//
int __declspec(naked) Z80_x86::Exec(int clocks)
{
#define ARG	[esp+24]
	__asm { push esi }
	__asm { mov esi,ecx }			// esi = this 
	__asm { push ecx }
	__asm { push ebp }
	__asm { push ebx }
	__asm { push edi }

	__asm { mov ecx,CPU.execcount }
	__asm { mov CLOCKCOUNT, ARG[0] }		// clocks
	__asm { mov CPU.startcount, ecx }
	__asm { add ecx,CLOCKCOUNT }
	__asm { mov eax,CPU.reg.r.w.af }
	__asm { mov CPU.execcount, ecx }
	__asm { mov INST,CPU.inst }
	__asm { neg CLOCKCOUNT }

	FETCHCODE(0)
	__asm { call O_INTR }			// 割り込み評価

OP_EXEC_2:
	FETCHCODE(1)
	__asm { test CLOCKCOUNT, CLOCKCOUNT }
	__asm { js OP_EXEC_2 }

	__asm { add CPU.execcount, CLOCKCOUNT }
	__asm { mov CPU.reg.r.w.af,eax	} // AF, R
	__asm { mov eax, ARG[0] }
	__asm { xor ecx, ecx }
	__asm { mov CPU.clockcount, ecx }
	__asm { add eax, CLOCKCOUNT }
	__asm { mov CPU.inst,edi }
	__asm { pop edi }
	__asm { pop ebx }
	__asm { pop ebp }
	__asm { pop ecx }
	__asm { pop esi }
	__asm { ret 4*1 }

	FETCHSUB(0);
	FETCHSUB(1);
#undef ARG
}

// ---------------------------------------------------------------------------
//	命令処理
//	前提条件: thiscall
//
int __declspec(naked) Z80_x86::ExecOne()
{
	__asm { push esi }
	__asm { mov esi,ecx }			// esi = this 
	__asm { push ecx }
	__asm { push ebp }
	__asm { push ebx }
	__asm { push edi }

	__asm { mov eax,CPU.reg.r.w.af }
	__asm { xchg ah, al }
	__asm { mov INSTLIM, 0 }
	__asm { mov INST,CPU.inst }
	__asm { xor CLOCKCOUNT, CLOCKCOUNT }

	FETCHCODE(0);

	__asm { add CPU.execcount, CLOCKCOUNT }
	__asm { mov CPU.inst,edi }
	__asm { xchg ah, al }
	__asm { mov CPU.reg.r.w.af,eax	} // AF, R
	__asm { mov eax, CLOCKCOUNT }
	__asm { pop edi }
	__asm { pop ebx }
	__asm { pop ebp }
	__asm { pop ecx }
	__asm { pop esi }
	__asm { ret 0 }

	FETCHSUB(0);
}

// ---------------------------------------------------------------------------
//	割り込みチェック
//
void __declspec(naked) Z80_x86::TestIntr()
{
	__asm { push esi }
	__asm { mov esi,ecx }			// esi = this 
	__asm { push ecx }
	__asm { push ebp }
	__asm { push ebx }
	__asm { push edi }

	__asm { mov eax,CPU.reg.r.w.af }
	__asm { xchg ah, al }
	__asm { mov INST,CPU.inst }
	__asm { xor CLOCKCOUNT, CLOCKCOUNT }

	__asm { call O_INTR }

	__asm { add CPU.execcount, CLOCKCOUNT }
	__asm { mov CPU.inst,edi }
	__asm { xchg ah, al }
	__asm { mov CPU.reg.r.w.af,eax	} // AF, R
	__asm { pop edi }
	__asm { pop ebx }
	__asm { pop ebp }
	__asm { pop ecx }
	__asm { pop esi }
	__asm { ret 0 }
}

// ---------------------------------------------------------------------------
//	命令処理
//	ecx = stop;
//	edx = delay;
//	esi = this
static void __declspec(naked) Exec0()
{
	__asm
	{
		mov ebp,CPU.execcount
		mov ebx,ebp
		mov currentz80, esi
		sub ebp,ecx			// if ebx(execcount) < ecx(stop) execute
		jns dontexec		// if ebp-ecx>=0 skip

		mov CPU.stopcount, ecx
		mov CPU.delaycount, edx
		sub ebx,ebp
		mov CPU.execcount, ebx

		mov INST,CPU.inst
		mov	INSTLIM,0

		cmp INSTWAIT, 0
		mov eax,CPU.reg.r.w.af
		jnz exec_1
exec_0:
		cmp INST,INSTLIM
		jnb indirect_0
		movzx edx,byte ptr [INST]
		inc INST
fetchend_0:
		add eax,1000000h
		call OpTable[edx*4]
		test CLOCKCOUNT,CLOCKCOUNT
		js exec_0
		jmp exec_e

exec_1:
	}
	FETCHCODE(1)
	__asm 
	{
		test CLOCKCOUNT, CLOCKCOUNT
		js exec_1

exec_e:
		add CPU.execcount, CLOCKCOUNT
		xor ecx,ecx
		mov CPU.clockcount, ecx
		mov CPU.reg.r.w.af,eax
		mov CPU.inst,INST

		mov ecx,CPU.stopcount
dontexec:
		ret
	}
	FETCHSUB(0);
	FETCHSUB(1);
}

// ---------------------------------------------------------------------------
//	命令処理
//	ecx = stop;
//	edx = delay;
//	esi = this
static void __declspec(naked) Exec1()
{
	__asm
	{
		mov ebp,CPU.execcount
		mov ebx,ebp
		mov currentz80, esi
		sub ebp,ecx			// if ebx(execcount) < ecx(stop) execute
		jns dontexec		// if ebp-ecx>=0 skip

		mov CPU.stopcount, ecx
		mov CPU.delaycount, edx
		sub ebx,ebp
		mov CPU.execcount, ebx

		mov	INSTLIM,0
		mov INST,CPU.inst
		sar ebp,1
		cmp INSTWAIT, 0

		mov eax,CPU.reg.r.w.af
		jnz exec_1
exec_0:
		cmp INST,INSTLIM
		jnb indirect_0
		movzx edx,byte ptr [INST]
		inc INST
fetchend_0:
		add eax,1000000h
		call OpTable[edx*4]
		test CLOCKCOUNT,CLOCKCOUNT
		js exec_0
		jmp exec_e

exec_1:
	}
	FETCHCODE(1)
	__asm 
	{
		test ebp,ebp
		js exec_1

exec_e:
		sal ebp,1
		add CPU.execcount, ebp
		xor ecx,ecx
		mov CPU.clockcount, ecx
		mov CPU.reg.r.w.af,eax
		mov CPU.inst,INST

		mov ecx,CPU.stopcount
dontexec:
		ret
	}
	FETCHSUB(0);
	FETCHSUB(1);
}

// ---------------------------------------------------------------------------
//
//
int __declspec(naked) __stdcall Z80_x86::ExecDual(Z80_x86* first, Z80_x86* second, int clocks)
{
#define ARG	[esp+24]
#define FIRST ARG[0]
#define SECOND ARG[4]
#define CLOCKS ARG[8]

	__asm
	{
		push esi
		push ecx
		push ebp
		push ebx
		push edi

		// execcount == GetCount()
		mov edi, FIRST
		mov	INSTLIM,0
		mov ebx, [edi]Z80_x86.execcount
		mov [edi]Z80_x86.startcount,ebx
		
		mov esi, SECOND
		mov	INSTLIM,0
		mov CPU.eshift,0
		mov CPU.delaycount, ebx
		xor CLOCKCOUNT, CLOCKCOUNT
			mov currentz80, esi
			mov eax, CPU.reg.r.w.af
			mov INST, CPU.inst
			FETCHCODE(0)
			call O_INTR
			mov CPU.inst, INST
			mov CPU.reg.r.w.af, eax
		mov ebx, CPU.execcount
		mov CPU.startcount,ebx
		add ebx, CLOCKCOUNT
		mov CPU.execcount, ebx

		mov esi, FIRST
		mov CPU.eshift,0
		mov CPU.delaycount, ebx
		xor CLOCKCOUNT,CLOCKCOUNT
		mov INST, CPU.inst
		mov edx,INSTBASE
		add edx,INST
		shr edx,PAGEBITS
		and edx,PAGEMASK
		mov ecx,WAITTBL[edx*4]
		mov INSTWAIT,ecx
			mov currentz80, esi
			mov eax, CPU.reg.r.w.af
			FETCHCODE(1)
			call O_INTR
			mov CPU.inst, INST
			mov CPU.reg.r.w.af, eax
		mov ebx,CPU.execcount
		add ebx,CLOCKCOUNT
		mov CPU.execcount, ebx
		mov ecx,CPU.delaycount

		// ebx = first->Count, ecx = second->Count
		cmp ecx, ebx
		js c2
		mov ecx, ebx
c2:
		// ecx = cbase
		push ecx
		add ecx, CLOCKS[4]
		
		mov edi, SECOND[4]
		mov edx, [edi]Z80_x86.execcount

		// ecx = stop
looop:
		mov esi, FIRST[4]
		cmp CPU.execcount,ecx	// ExecCount >= stopならスキップ
		jns x1

		call Exec0
		mov edx, CPU.execcount

		mov esi, SECOND[4]
x2:
		call Exec0
		mov edx, CPU.execcount
		
		jmp looop

x1:
		mov esi, SECOND[4]
		cmp CPU.execcount, ecx
		js x2
		mov eax,ecx
		pop edx
		sub eax,edx
		xor ecx,ecx
		mov currentz80, ecx

		pop edi
		pop ebx
		pop ebp
		pop ecx
		pop esi
		ret 12
	}
	FETCHSUB(0);
	FETCHSUB(1);
#undef ARG
#undef FIRST
#undef SECOND
#undef CLOCKS
}

// ---------------------------------------------------------------------------
//
//
int __declspec(naked) __stdcall Z80_x86::ExecDual2(Z80_x86* first, Z80_x86* second, int clocks)
{
#define ARG	[esp+24]
#define FIRST ARG[0]
#define SECOND ARG[4]
#define CLOCKS ARG[8]

	__asm
	{
		push esi
		push ecx
		push ebp
		push ebx
		push edi

		// execcount == GetCount()
		mov edi, FIRST
		mov	INSTLIM,0
		mov ebx, [edi]Z80_x86.execcount
		mov [edi]Z80_x86.startcount,ebx
		
		mov esi, SECOND
		mov	INSTLIM,0
		mov CPU.eshift,1
		mov CPU.delaycount, ebx
		xor CLOCKCOUNT, CLOCKCOUNT
		mov INST, CPU.inst
//		mov edx,INSTBASE
//		add edx,INST
//		shr edx,PAGEBITS-PAGESHIFT
//		and edx,PAGEMASK << PAGESHIFT
//		mov ecx,RDPAGES[edx].wait
//		mov INSTWAIT,ecx
			mov currentz80, esi
			mov eax, CPU.reg.r.w.af
			FETCHCODE(0)
			call O_INTR
			mov CPU.inst, INST
			mov CPU.reg.r.w.af, eax
		mov ebx, CPU.execcount
		mov CPU.startcount,ebx
		lea ebx,[ebx+ebp*2]
		mov CPU.execcount, ebx

		mov esi, FIRST
		mov CPU.eshift,0
		mov CPU.delaycount, ebx
		xor CLOCKCOUNT,CLOCKCOUNT
		mov INST, CPU.inst
		mov edx,INSTBASE
		add edx,INST
		shr edx,PAGEBITS
		and edx,PAGEMASK
		mov ecx,WAITTBL[edx*4]
		mov INSTWAIT,ecx
			mov currentz80, esi
			mov eax, CPU.reg.r.w.af
			FETCHCODE(1)
			call O_INTR
			mov CPU.inst, INST
			mov CPU.reg.r.w.af, eax
		mov ebx,CPU.execcount
		add ebx,CLOCKCOUNT
		mov CPU.execcount, ebx
		mov ecx,CPU.delaycount

		// ebx = first->Count, ecx = second->Count
		cmp ecx, ebx
		js c2
		mov ecx, ebx
c2:
		// ecx = cbase
		push ecx
		add ecx, CLOCKS[4]
		
		mov edi, SECOND[4]
		mov edx, [edi]Z80_x86.execcount

		// ecx = stop
looop:
		mov esi, FIRST[4]
		cmp CPU.execcount,ecx	// ExecCount >= stopならスキップ
		jns x1

		call Exec0
		mov edx, CPU.execcount

		mov esi, SECOND[4]
x2:
		call Exec1
		mov edx, CPU.execcount
		
		jmp looop

x1:
		mov esi, SECOND[4]
		cmp CPU.execcount, ecx
		js x2
		mov eax,ecx
		pop edx
		sub eax,edx
		xor ecx,ecx
		mov currentz80, ecx

		pop edi
		pop ebx
		pop ebp
		pop ecx
		pop esi
		ret 12
	}
	FETCHSUB(0);
	FETCHSUB(1);
#undef ARG
#undef FIRST
#undef SECOND
#undef CLOCKS
}

// ---------------------------------------------------------------------------
//
//
int __declspec(naked) __stdcall 
Z80_x86::ExecSingle(Z80_x86* first, Z80_x86* second, int clocks)
{
#define ARG	[esp+24]
#define FIRST ARG[0]
#define SECOND ARG[4]
#define CLOCKS ARG[8]

	__asm
	{
		push esi
		push ecx
		push ebp
		push ebx
		push edi

		// execcount == GetCount()
		mov esi, FIRST
		mov ebx, CPU.execcount
		mov CPU.startcount,ebx

		mov CPU.eshift,0
		mov CPU.delaycount, ebx
		xor CLOCKCOUNT,CLOCKCOUNT
		mov INST, CPU.inst
		mov edx,INSTBASE
		add edx,INST
		shr edx,PAGEBITS
		and edx,PAGEMASK
		mov ecx,WAITTBL[edx*4]
		mov INSTWAIT,ecx
		mov currentz80, esi
		mov eax, CPU.reg.r.w.af
		FETCHCODE(0)
		call O_INTR
		mov CPU.inst, INST
		mov CPU.reg.r.w.af, eax
		
		mov edx,CPU.execcount
		mov ebx,CLOCKS
		lea ecx,[edx+ebx]
		push edx
		call Exec0
		pop edx

		mov edi,SECOND
		mov eax,CPU.execcount
		mov [edi]Z80_x86.execcount, eax
		sub eax,edx
		
		xor ecx,ecx
		mov currentz80, ecx

		pop edi
		pop ebx
		pop ebp
		pop ecx
		pop esi
		ret 12
	}
	FETCHSUB(0);
#undef ARG
#undef FIRST
#undef SECOND
#undef CLOCKS
}

// ---------------------------------------------------------------------------

void Z80_x86::StopDual(int clocks)
{
	if (currentz80)
		currentz80->Stop(clocks);
}

// ---------------------------------------------------------------------------
//	割り込み制御命令 ---------------------------------------------------------

static OPFUNC(DI)
{
	CLK(4, 1);
	__asm { xor cl,cl }
	__asm { mov IFF1,cl }
	__asm { mov IFF2,cl }
	__asm { ret }
}

static OPFUNC(EI)
{
	CLK(4, 1)
	FETCH8
	__asm { mov ebx,0f7h }					// 直後の１命令を実行する
	__asm { and ebx,edx }					// 但し DI または EI 命令であったら
	__asm { cmp ebx,0f3h }					// 何もしない
	__asm { jz dont_exec }
	__asm { add eax,1000000h }
	__asm { call OpTable[edx*4] }

	__asm { mov cl,1 }
	__asm { mov IFF1,cl }
	__asm { mov IFF2,cl }
	__asm { call O_INTR }					// 割り込みチェック＆割り込み実行
	__asm { ret }
	
	FETCH8SUB; 

dont_exec:
	PCDEC1;
	__asm { ret }
}

static OPFUNC(OUTINTR)
{
	__asm
	{
		mov dl,IFF1							// Is interrupt pending ?
		and dl,CPU.intr
		jnz process
		ret
	}
process:
	CLK(4, 1);
	FETCH8
	__asm
	{
		cmp edx,0edh
		jz insted
		cmp edx,0d3h
		jz instd3
		add eax,1000000h
		call OpTable[edx*4]
		call O_INTR
		ret
	}
	FETCH8SUB;

instd3:
	PCDEC1;
	__asm { ret }

insted:
	__asm
	{
		call Fetcher8
		mov ecx,0c7h
		mov ebx,0e7h
		and ecx,edx
		and ebx,edx
		test ecx,41h
		jz	instout
		test ebx,0a3h
		jz	instout

		mov PQ,INSTWAIT
		sub PQ,CLOCKCOUNT
		sub UV,40h
		add eax,02000000h
		cmp UV,80h
		jnb none
		jmp OpTableED[UV*4]
	}
none:
	CLK(8, 2);
	__asm { ret }

instout:
	PCDEC2;
	__asm { ret }
}

// ---------------------------------------------------------------------------
//	構築・破棄
//
Z80_x86::Z80_x86(const ID& id)
: Device(id)
{
	Reset();
	execcount = 0;
	clockcount = 0;
	delaycount = 0;
	stopcount = 0;
	eshift = 0;
	memset(waittable, 0, sizeof(int) * (0x10000 >> MemoryManager::pagebits));
}

Z80_x86::~Z80_x86()
{
}

// ---------------------------------------------------------------------------
//	Stop
//	
void Z80_x86::Stop(int count)
{
	stopcount = execcount = GetCount() + count;
	clockcount = -count >> eshift;
}

// ---------------------------------------------------------------------------
//	初期化
//
bool Z80_x86::Init(MemoryManager* mm, IOBus* bus, int iack)
{
	assert(MemoryManager::pagebits == PAGEBITS);	// check
	if (MemoryManager::pagebits != PAGEBITS)		// check for release build
		return false;
	
	ins = bus->GetIns();
	outs = bus->GetOuts();
	ioflags = bus->GetFlags();
	intack = iack;
	return true;
}

// ---------------------------------------------------------------------------
//	リセット
//	bank - 割り込み番号
//
void IOCALL Z80_x86::Reset(uint, uint)
{
	memset(&reg, 0, sizeof(reg));

	inst = instbase = instlim = 0;
	instpage = (uint8*) ~0;
	intr = 0;
	waitstate = 0;
	execcount = 0;
}

// ---------------------------------------------------------------------------
//	マスカブル割込み要求
//
void IOCALL Z80_x86::IRQ(uint, uint d)
{
	intr = d;
}

// ---------------------------------------------------------------------------
//	ノンマスカブル割込み要求
//	未実装
//	
void IOCALL Z80_x86::NMI(uint, uint)
{
}

// ---------------------------------------------------------------------------
//	Exec してから経過したクロック数を取得
//	(ExecOne では未対応)
//
int Z80_x86::GetCCount()
{
	return currentz80 ? currentz80->GetCount() - currentz80->startcount : 0;
}

// ---------------------------------------------------------------------------
//	状態保存
//
bool IFCALL Z80_x86::SaveStatus(uint8* s)
{
	CPUState* status = (CPUState*) s;
	status->rev = ssrev;
	reg.pc = GetPC();
	status->reg = reg;
	status->reg.r.b.a = reg.r.w.af & 0xff;
	status->reg.r.b.flags = (reg.r.w.af >> 8) & 0xff;
	status->reg.rreg = (reg.r.w.af >> 24) & 0xff;
	status->intr = intr;
	status->flagn = flagn;
	status->waitstate = waitstate;
	status->execcount = execcount;
	return true;
}

// ---------------------------------------------------------------------------
//	状態復元
//
bool IFCALL Z80_x86::LoadStatus(const uint8* s)
{
	const CPUState* status = (const CPUState*) s;
	if (status->rev != ssrev)
		return false;
	reg = status->reg;
	reg.r.w.af = (status->reg.r.b.a & 0xff) | (status->reg.r.b.flags << 8) | (status->reg.rreg << 24);
	intr = status->intr;
	flagn = status->flagn;
	waitstate = status->waitstate;
	execcount = status->execcount;
	
	inst		= (uint8*) reg.pc;
	instbase	= (uint8*) 0;
	instlim		= (uint8*) 0;
	instpage	= (uint8*) ~0;
	return true;
}

// ---------------------------------------------------------------------------
//	Device descriptor
//	
const Device::Descriptor Z80_x86::descriptor =
{
	0, outdef
};

const Device::OutFuncPtr Z80_x86::outdef[] =
{
	STATIC_CAST(Device::OutFuncPtr, &Reset),
	STATIC_CAST(Device::OutFuncPtr, &IRQ),
	STATIC_CAST(Device::OutFuncPtr, &NMI),
};

#endif // USE_Z80_X86
