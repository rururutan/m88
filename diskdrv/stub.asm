; ----------------------------------------------------------------------------
;	RAM に作業用プログラムを転送
;
SetupRAMRoutine:
		push	hl
		ld	hl,block1start
		ld	de,block1
		ld	bc,block1len
		ldir
		in	a,(71h)
		ld	(ROMBank),a
		pop	hl
		ret

; ----------------------------------------------------------------------------
;	常駐スタブ
;
block0start	equ	$
		org	0f310h
block0		equ	$

CMDStub:
		ld	a,0
		out	(71h),a
		call	CMDEntry
		ld	a,0ffh
		out	(71h),a
		ret

block0len	equ	$-block0
		org	block0start + block0len

; ----------------------------------------------------------------------------
;	非常駐 RAM ルーチン
;
block1start	equ	$
		org	tempramworkarea
block1		equ	$
		
ROMCALL:
		ex	(sp),hl
		push	af
		ld	a,(hl)
		inc	hl
		ld	(ROMCALL_1+1),a
		ld	a,(hl)
		inc	hl
		ld	(ROMCALL_1+2),a
		ld	a,0ffh
		out	(71h),a
		pop	af
		ex	(sp),hl
ROMCALL_1:	call	0
EROMRET:	push	af
		ld	a,0
ROMBank		equ	$-1
		out	(71h),a
		pop	af
		ret

block1end	equ	$
block1len	equ	$-block1
		org	block1start + block1len

