; ----------------------------------------------------------------------------
;	ディスクアクセス ROM
;
;	使用するメモリ領域
;	常駐:	f310-f31b	(予備割り込みベクタ内)
;	非常駐:	f04f-f086	(PAINT/CIRCLE ワーク)
;		e9b9-eaba	(行入力バッファ)
;
;	使用可能命令
;
;	CMD LOAD "ファイル名" [,R]
;		ファイル名で指定された BASIC プログラムを読み込む
;		ASCII/中間コードのどちらのファイルも対応している
;
;	CMD SAVE "ファイル名" [,A/B]
;		BASIC プログラムを指定されたファイル名で保存
;		オプション無しの場合 ASCII テキストとして保存される．
;		(,A オプションも同様)
;		中間コードのままで保存するには ,B オプションをつける
;
;	CMD BLOAD "ファイル名" [,読み込みアドレス] [,R]
;		機械語ファイルを読み込む
;
;	CMD BSAVE "ファイル名", 開始アドレス, 長さ
;		機械語ファイルの保存
;

SNERR		equ	0393h
FCERR		equ	0b06h

DIOPORT		equ	0d0h
DIOCMD		equ	DIOPORT+0
DIOSTAT		equ	DIOPORT+0
DIODAT		equ	DIOPORT+1

LINKER		equ	05bdh
LINEA2N		equ	01bbbh
TEXTW2R		equ	44d5h
TEXTR2W		equ	44a4h

TXTTAB		equ	0e658h
TXTEND		equ	0eb18h

temp0		equ	0eab9h

tempramworkarea	equ	0f04fh
RAMCodeArea	equ	0e9b9h

		org	6000h

;		org	$-4
;bloadhdr	dw	$+4
;		dw	codeend


eromid		db	'R4'

; ----------------------------------------------------------------------------
;	初期化処理
;
erominit:
		jr	initialize

TitleMsg	db	"M88 Disk I/O Extention 0.23",13,10,0

initialize:
		call	DetectHW
		ret	c
		
		call	SetupRAMRoutine
		
		ld	hl,TitleMsg
		call	putmsg
		
		
		ld	hl,block0start
		ld	de,block0
		ld	bc,block0len
		ldir
		in	a,(71h)
		ld	(CMDStub+1),a

;	CMD フックが使用されているかチェック
		ld	hl,(0eeb7h)
		ld	de,4dc1h
		or	a
		sbc	hl,de
		jr	nz, init_err
		
;       拡張コマンドにフック
		ld	hl,CMDStub
		ld	(0eeb7h),hl
		ret
		
init_err:
		ld	hl,InstallFailMsg
		call	putmsg
		ret

InstallFailMsg	db	"CMD extention is alredy in use. Installation aborted.",13,10,0

	incl "stub.asm"

; ----------------------------------------------------------------------------
;	拡張コマンドエントリ
;
CMDEntry:
		call	SetupRAMRoutine
		
		ld	a,(hl)
		inc	hl
		cp	0c8h
		jp	z,LSAVE
		cp	0c1h
		jp	z,LLOAD
		cp	0d4h
		jp	z,LBLOAD
		cp	0d5h
		jp	z,LBSAVE
		call	ROMCALL
		dw	SNERR

; ----------------------------------------------------------------------------
;	拡張ハード検出
;
DetectHW:
		ld	a,80h
		out	(DIOCMD),a
		in	a,(DIOSTAT)
		cp	1
		jr	nz,detecthw_e
		xor	a
		out	(DIODAT),a
		in	a,(DIOSTAT)
		or	a
		ret	z
detecthw_e:
		scf
		ret

; ----------------------------------------------------------------------------
;	ファイルネームの取得
;	
getfilename:
		call	ROMCALL
		dw	11d3h		; eval
		push	hl
		call	ROMCALL
		dw	56c9h
		ld	b,(hl)
		inc	b
		dec	b
		jp	z,getfilename_e
		inc	hl
		ld	e,(hl)
		inc	hl
		ld	d,(hl)
		
		ld	a,80h		; set filename
		out	(DIOCMD),a
		ld	a,b
		out	(DIODAT),a
		
getfilename_1:
		ld	a,(de)
		out	(DIODAT),a
		inc	de
		djnz	getfilename_1
		
		pop	hl
		ret
		
getfilename_e:
		call	ROMCALL
		dw	FCERR

 incl "load.asm"
 incl "save.asm"
 incl "misc.asm"


codeend		equ	$
 
		end
