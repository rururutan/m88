; ----------------------------------------------------------------------------
;	LOAD Command
;	LOAD "filename" [,R]
;
LLOAD:
		call	LoadStart
		push	bc
		ld	bc,0200h
lload_0:
		push	bc
		call	ROMCALL
		dw	11h
		jr	z,lload_2
		cp	','
		jr	nz,lload_2
		
		call	ROMCALL
		dw	10h
		pop	bc
		jr	z,lload_6
		inc	hl
		or	20h
		cp	'a'
		jr	z,lload_3
		cp	'r'
		jr	z,lload_4
lload_6:
SyntaxError:
		ld	e,2
		jp	ErrorExit
lload_3:
		set	0,c
		db	11h
lload_4:
		set	1,c
lload_5:
		djnz	lload_0
		db	3eh
lload_2:
		pop	bc
		ld	a,c
		pop	bc
		push	af
		push	hl
	
	; ASCII/BINARY 判別
		inc	b
		dec	b
		jr	nz,lload_7
		ld	a,c
		cp	2
		jr	c,lload_8
lload_7:
		in	a,(DIODAT)
		ld	e,a
		in	a,(DIODAT)
		ld	d,a
		cp	' '
		jr	z,lload_9
		sub	'0'
		cp	10
		jr	nc,lload_10
lload_9:
		ld	a,e
		sub	'0'
		cp	10
		jr	c,asciiload
	
	; BINARY LOAD
lload_10:
		ld	a,b
		rlca
		jp	c,OutOfMemory
		
		ld	hl,(TXTTAB)
		push	hl
		add	hl,bc
		pop	hl
		jp	pe,OutOfMemory
		
		ld	(hl),e
		inc	hl
		dec	bc
		ld	(hl),d
		inc	hl
		dec	bc
lload_8:
		call	LoadMain
		
		call	ROMCALL
		dw	LINKER
		call	ROMCALL
		dw	LINEA2N
		call	ROMCALL
		dw	TEXTW2R
		inc	hl
		ld	(TXTEND),hl
		pop	hl
		call	CheckError
		pop	af
		bit	1,a
		ret	z
		
		pop	hl
		pop	bc
		ld	bc,0b7eh
		push	bc
		jp	(hl)

; ----------------------------------------------------------------------------
;	ASCII BASIC プログラムロード
;
asciiload:
		ld	(0e9b8h),de	; 先読みした２バイトを保存
		
		ld	hl,(TXTTAB)	; NEW 的処理
		dec	hl
		xor	a
		ld	(hl),a
		inc	hl
		ld	(hl),a
		inc	hl
		ld	(hl),a
		inc	hl
		ld	(TXTEND),hl
		ld	(0ec3bh),a	; TROFF
		ld	(0eb00h),a	; AUTO
		ld	(0ec29h),a	; PROTECT
		ld	(0eaffh),a	; 
		
		call	loadhookvector
		
		ld	hl,0e9b8h+2	; 最初は２バイト分ずらす
		ld	b,0fch
		jr	asciiload_2
asciiload_1:
		ld	hl,0e9b8h		
		ld	b,0feh
asciiload_2:				; まず制御文字を飛ばす
		in	a,(DIOSTAT)
		and	4
		jr	z,asciiload_e	; EOF チェック
		in	a,(DIODAT)
		cp	20h		; skip control char
		jr	c,asciiload_2
		ld	(hl),a
		inc	hl
asciiload_3:
		in	a,(DIOSTAT)	; 文字列を読み込む
		and	4
		jr	z,asciiload_e
		in	a,(DIODAT)
		cp	9		; tab は空白文字ってことで...
		jr	z,asciiload_6
		cp	20h
		jr	c,asciiload_4	; 制御文字が来た時点で１行終わり
asciiload_6:
		ld	(hl),a
		inc	hl
		djnz	asciiload_3
		ld	e,23		; Line buffer overflow
		jp	ErrorExit

asciiload_4:
		ld	(hl),0
		
		ld	hl,0e9b8h-1	; 文字列
asciiload_5:
		inc	hl		; 空白飛ばし
		ld	a,(hl)
		cp	' '
		jr	z,asciiload_5
		cp	9
		jr	z,asciiload_5
		
		ld	a,(hl)
		sub	'0'		; プログラムであることを確認
		cp	10
		ld	e,57		; Direct statement in file
		jp	nc,ErrorExit
		call	ROMCALL
		dw	4e7h
		call	ROMCALL		; LINKER
		dw	5c1h
		jr	asciiload_1
		
asciiload_e:
		call	loadresetvector
		
		pop	hl
		call	CheckError	; エラーチェック
		pop	af
		pop	hl
		pop	bc
		bit	1,a		; ,R オプション有りなら
		ld	bc,0b7eh	; RUN
		jr	nz,asciiload_r	; 無しなら
		ld	bc,050e6h	; CLEAR
asciiload_r:				; のエントリにぬける
		push	bc
		jp	(hl)

; ----------------------------------------------------------------------------

loadhookvector:
		ld	hl,loadsub_blk
		ld	de,loadsub_ptr
		ld	bc,loadsub_len
		ldir
		ld	hl,0ed30h
		ld	de,load_oldvectors
		ld	bc,3
		ldir
		ld	hl,0edc9h
		ld	c,3
		ldir
		
		ld	hl,load_newvectors
		call	loadsetvector
		ret
		
load_newvectors:
		pop	de			; ED30
		pop	de
		ret
		jp	load_errhook		; EDC9


loadsub_blk	equ	$
		org	block1end
loadsub_ptr
load_errhook:
		exx
		ex	af,af'
		xor	a
		out	(DIOCMD),a
		call	loadresetvector
		exx
		jp	0edc9h
loadresetvector:
		ld	hl,load_oldvectors
loadsetvector:
		ld	de,0ed30h
		ld	bc,3
		ldir
		ld	de,0edc9h
		ld	c,3
		ldir
		ret

loadsub_len	equ	$-loadsub_ptr
load_oldvectors	equ	0e000h
		org	loadsub_blk + loadsub_len
		
; ----------------------------------------------------------------------------
;	LOAD 共通処理
;
LoadStart:
		call	getfilename
		ld	a,81h
		out	(DIOCMD),a
		in	a,(DIODAT)
		ld	c,a
		in	a,(DIODAT)
		ld	b,a
;		jr	IsOk

IsOk:
		in	a,(DIOSTAT)
		rrca
		ret	c
CheckError:
		ld	a,83h
		out	(DIOCMD),a
		in	a,(DIODAT)
		or	a
		ret	z
		ld	e,a
ErrorExit:
		call	ROMCALL
		dw	3b3h

OutOfMemory:
		ld	e,7		; Out of memory
		jr	ErrorExit



; ----------------------------------------------------------------------------
;	BLOAD Command
;	bload "prog" [,addr]
;
LBLOAD:
		call	LoadStart
		
		in	a,(DIODAT)
		ld	e,a
		in	a,(DIODAT)
		ld	d,a
		in	a,(DIODAT)
		sub	e
		ld	c,a
		in	a,(DIODAT)
		sbc	a,d
		ld	b,a
;	jr lbload_1
		ld	a,(hl)
		cp	','
		jr	nz,lbload_1
		inc	hl
		ld	a,(hl)
		or	20h
		cp	'r'
		jr	z,lbload_2
		push	bc
		call	ROMCALL
		dw	1896h
		pop	bc
		db	3eh
lbload_2:
		dec	hl
lbload_1:
		ex	de,hl
		ld	(temp0),hl
		call	LoadMain
		ex	de,hl
		call	CheckError
		
		ld	a,(hl)
		cp	','
		ret	nz
		inc	hl
		ld	a,(hl)
		or	20h
		cp	'r'
		jp	nz,SyntaxError
		inc	hl
		push	hl
		ld	a,0c3h
		ld	(temp0-1),a
		call	ROMCALL
		dw	temp0-1
		pop	hl
		ret

; ----------------------------------------------------------------------------
;
;
LoadMain:
		in	a,(DIODAT)
		ld	(hl),a
		cpi
		ret	po
		jr	LoadMain

