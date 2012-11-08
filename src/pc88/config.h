// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: config.h,v 1.23 2003/09/28 14:35:35 cisc Exp $

#pragma once

#include "types.h"

// ---------------------------------------------------------------------------

namespace PC8801
{

class Config
{
public:
	enum BASICMode
	{
		// bit0 H/L
		// bit1 N/N80 (bit5=0)
		// bit4 V1/V2
		// bit5 N/N88
		// bit6 CDROM 有効
		N80 = 0x00, N802 = 0x02, N80V2 = 0x12,
		N88V1 = 0x20, N88V1H = 0x21, N88V2 = 0x31, 
		N88V2CD = 0x71,
	};
	enum KeyType
	{
		AT106=0, PC98=1, AT101=2
	};
	enum CPUType
	{
		ms11 = 0, ms21, msauto,
	};

	enum Flags
	{
		subcpucontrol	= 1 <<  0,	// Sub CPU の駆動を制御する
		savedirectory	= 1 <<  1,	// 起動時に前回終了時のディレクトリに移動
		fullspeed		= 1 <<  2,	// 全力動作
		enablepad		= 1 <<  3,	// パッド有効
		enableopna		= 1 <<  4,	// OPNA モード (44h)
		watchregister	= 1 <<  5,	// レジスタ表示
		askbeforereset	= 1 <<  6,	// 終了・リセット時に確認
		enablepcg		= 1 <<  7,	// PCG 系のフォント書き換えを有効
		fv15k			= 1 <<  8,	// 15KHz モニターモード
		cpuburst        = 1 <<  9,	// ノーウェイト
		suppressmenu	= 1 << 10,	// ALT を GRPH に
		cpuclockmode	= 1 << 11,	// クロック単位で切り替え
		usearrowfor10	= 1 << 12,	// 方向キーをテンキーに
		swappadbuttons	= 1 << 13,	// パッドのボタンを入れ替え
		disablesing		= 1 << 14,	// CMD SING 無効
		digitalpalette	= 1 << 15,	// ディジタルパレットモード
		useqpc			= 1 << 16,	// QueryPerformanceCounter つかう
		force480		= 1 << 17,	// 全画面を常に 640x480 で
		opnona8			= 1 << 18,	// OPN (a8h)
		opnaona8		= 1 << 19,	// OPNA (a8h)
		drawprioritylow	= 1 << 20,	// 描画の優先度を落とす
		disablef12reset = 1 << 21,  // F12 を RESET として使用しない(COPY キーになる)
		fullline		= 1 << 22,  // 偶数ライン表示
		showstatusbar	= 1 << 23,	// ステータスバー表示
		showfdcstatus	= 1 << 24,	// FDC のステータスを表示
		enablewait		= 1 << 25,	// Wait をつける
		enablemouse		= 1 << 26,	// Mouse を使用
		mousejoymode	= 1 << 27,	// Mouse をジョイスティックモードで使用
		specialpalette	= 1 << 28,	// デバックパレットモード
		mixsoundalways	= 1 << 29,	// 処理が重い時も音の合成を続ける
		precisemixing	= 1 << 30,	// 高精度な合成を行う
	};
	enum Flag2
	{
		disableopn44	= 1 <<  0,	// OPN(44h) を無効化 (V2 モード時は OPN)
		usewaveoutdrv	= 1 <<	1,	// PCM の再生に waveOut を使用する
		mask0			= 1 <<  2,	// 選択表示モード
		mask1			= 1 <<  3,
		mask2			= 1 <<  4,
		genscrnshotname = 1 <<  5,	// スクリーンショットファイル名を自動生成
		usefmclock		= 1 <<  6,	// FM 音源の合成に本来のクロックを使用
		compresssnapshot= 1 <<  7,  // スナップショットを圧縮する
		synctovsync		= 1 <<  8,	// 全画面モード時 vsync と同期する
		showplacesbar	= 1 <<  9,	// ファイルダイアログで PLACESBAR を表示する
		lpfenable		= 1 << 10,	// LPF を使ってみる
		fddnowait		= 1 << 11,	// FDD ノーウェイト
		usedsnotify		= 1 << 12,
		saveposition	= 1 << 13,	// 起動時に前回終了時のウインドウ位置を復元
	};

	int flags;
	int flag2;
	int clock;
	int speed;
	int refreshtiming;
	int mainsubratio;
	int opnclock;
	int sound;
	int erambanks;
	int keytype;
	int volfm, volssg, voladpcm, volrhythm;
	int volbd, volsd, voltop, volhh, voltom, volrim;
	int dipsw;
	uint soundbuffer;
	uint mousesensibility;
	int cpumode;
	uint lpffc;				// LPF のカットオフ周波数 (Hz)
	uint lpforder;
	int romeolatency;
	int winposx;
	int winposy;

	BASICMode basicmode;

	// 15kHz モードの判定を行う．
	// (条件: option 又は N80/SR モード時)
	bool IsFV15k() const { return (basicmode & 2) || (flags & fv15k); }
};

}

