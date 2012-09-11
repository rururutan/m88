; ----------------------------------------------------------------------------
;	SAVE Command
;	SAVE "filename" [,A/B]
;		
LSAVE:
		call	getfilename
		call	ROMCALL
		dw	11h
		cp	','
		jr	nz,lsave_1
		
		inc 	hl
		ld	a,(hl)
		inc	hl
		or	20h
		cp	'a'
		jr	z,lsave_1
		cp	'b'
		jp	nz,SyntaxError

lsave_0:
;	BINARY SAVE
		push	hl
		ld	de,(TXTEND)
		dec	de
		ld	hl,(TXTTAB)
		call	savedata
		pop	hl
		jp	CheckError
		
lsave_1:

;	ASCII SAVE

		in	a,(70h)
		push	af
		push	hl
		
		ld	a,82h		; file write
		out	(DIOCMD),a
		call	IsOk
		ld	a,0ffh
		out	(DIODAT),a
		out	(DIODAT),a
		
		call	ROMCALL
		dw	LINEA2N
		ld	hl,(TXTTAB)
lsave_2:
		ld	a,h
		out	(70h),a
		ld	h,80h
		
		ld	e,(hl)
		inc	hl
		ld	d,(hl)
		inc	hl
		ld	a,d
		or	e
		jr	z,lsave_e
		
		push	de		; = 次の行の開始点
		ld	e,(hl)
		inc	hl
		ld	d,(hl)
		inc	hl		; de = 行番号
		push	hl
		ex	de,hl
		ld	de,0e9b9h
		call	num2dec
		ld	a,' '
		ld	(de),a
		inc	de
		
		ld	hl,0e9b9h
		ld	a,e
		sub	l
		ld	b,a
		
lsave_4:
		ld	a,(hl)
		inc	hl
		out	(DIODAT),a
		djnz	lsave_4
		pop	hl
		call	ROMCALL
		dw	194ch		; 中間言語 -> テキスト変換
		ld	hl,0e9b9h
		ld	c,DIODAT
lsave_3:
		ld	a,(hl)
		inc	hl
		or	a
		jr	z,lsave_5
		out	(c),a
		jr	lsave_3
lsave_5:
		ld	de,0d0ah
		out	(c),d
		out	(c),e
		pop	hl
		jr	lsave_2		; 次の行
		
lsave_e:
		ld	a,084h
		out	(DIOCMD),a
		pop	hl
		pop	af
		out	(070h),a
		jp	CheckError

; ----------------------------------------------------------------------------
;	BSAVE Command
;	BSAVE "filename",ADR,LEN
;		
LBSAVE:
		call	getfilename
		ld	a,82h		; file write
		out	(DIOCMD),a
		call	IsOk
		call	savedata_init
		call	bsavesub
		jp	CheckError
		

; ----------------------------------------------------------------------------
;	データ送信
;	hl	開始アドレス
;	de	データサイズ
;
savedata:
		push	de
		call	savedata_init
		pop	de
		ld	a,82h		; file write
		out	(DIOCMD),a
		call	IsOk
		
		ld	c,DIODAT
		out	(c),e
		out	(c),d
		call	savedatasub
		db	0edh, 71h	; out (c),0
		db	0edh, 71h	; out (c),0
		ret

savedata_init:
		push	hl
		ld	hl,savedatasub_blk
		ld	de,RAMCodeArea
		ld	bc,savedatasub_len
		ldir
		pop	hl
		ret

savedatasub_blk	equ	$
		org	RAMCodeArea
		
savedatasub:
		di
		ld	a,(0e6c2h)
		push	af
		or	2
		out	(31h),a
savedatasub_1:
		outi
		dec	de
		ld	a,d
		or	e
		jr	nz,savedatasub_1
		
		pop	af
		out	(31h),a
		ei
		ret
		
bsavesub:
		ld	a,0ffh
		out	(71h),a
		rst	8
		db	','
		call	1896h
		push	de
		db	','
		call	1896h
		pop	bc
		
		push	hl
		ld	a,c
		ld	c,DIODAT
		
		ld	hl,4
		add	hl,de
		out	(c),l
		out	(c),h
		out	(c),a
		out	(c),b
		ld	h,b
		ld	l,a
		add	hl,de
		out	(c),l
		out	(c),h
		ld	h,b
		ld	l,a
		
		call	savedatasub
		db	0edh, 71h	; out (c),0
		db	0edh, 71h	; out (c),0
		pop	hl

 		jp	EROMRET
		
savedatasub_len	equ	$-RAMCodeArea
		org	savedatasub_blk + savedatasub_len


