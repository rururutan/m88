


//			type	 name				default value	下限	上限
DECLARE_CONFIG_INT	(BASICMode,			0x31,			0,		0xff)
DECLARE_CONFIG_INT	(ERAMBanks,			0,				0,		256)
DECLARE_CONFIG_INT	(DipSwitch,			1829,			0,		INT_MAX)


// CPU
DECLARE_CONFIG_INT	(CPUClock,			40,				1,		1000)
DECLARE_CONFIG_SYM	(CPUMSSpeed,		CPUMSSpeedType,	msauto)


DECLARE_CONFIG_BOOL	(SubCPUControl,		true)								// Sub CPU の駆動を制御する
DECLARE_CONFIG_BOOL	(FullSpeed,			false)								// 全力動作
DECLARE_CONFIG_INT	(Speed,				1000,			500,	2000)
DECLARE_CONFIG_BOOL	(NoWaitMode,		false)								// ノーウェイト
DECLARE_CONFIG_BOOL	(FDDWait,			true)								// FDD ウェイト
DECLARE_CONFIG_BOOL	(CPUWait,			true)								// ウェイトのエミュレーション


DECLARE_CONFIG_BOOL	(EnablePCG,			false)								// PCG 系のフォント書き換えを有効
DECLARE_CONFIG_SYM	(DisplayMode,		DispModeType,	24k)				// 15KHz モニターモード
DECLARE_CONFIG_SYM	(PaletteMode,		PalModeType,	Analogue)			// パレットモード
DECLARE_CONFIG_BOOL	(Force480,			false)								// 全画面を常に 640x480 で

DECLARE_CONFIG_BOOL	(FullLine,			true)								// 偶数ライン表示
DECLARE_CONFIG_BOOL	(DrawPriorityLow,	false)								// 描画の優先度を落とす
DECLARE_CONFIG_INT	(DrawInterval,		3,				1,		4)



DECLARE_CONFIG_SYM	(SoundDriver,		SoundDriverType,DirectSound)		// PCM の再生に使用するドライバ
DECLARE_CONFIG_INT	(SoundBuffer,		200,			50,		1000)
DECLARE_CONFIG_BOOL	(SoundPriorityHigh,	false)								// 処理が重い時も音の合成を続ける

DECLARE_CONFIG_BOOL	(EnableCMDSING,		true)								// CMD SING 有効
DECLARE_CONFIG_SYM	(OpnType44h,		OpnType,		OPN)				// OPN (44h)
DECLARE_CONFIG_SYM	(OpnTypeA8h,		OpnType,		None)				// OPN (a8h)

DECLARE_CONFIG_INT	(PCMRate,			22100,			8000,	96000)
DECLARE_CONFIG_BOOL	(FMMix55k,			true)								// FM 音源の合成に本来のクロックを使用
DECLARE_CONFIG_BOOL	(PreciseMixing,		true)								// 高精度な合成を行う
DECLARE_CONFIG_BOOL	(EnableLPF,			false)								// LPF を使ってみる
DECLARE_CONFIG_INT	(LPFCutoff,			9000,			3000,	48000)
DECLARE_CONFIG_INT	(LPFOrder,			4,				2,		16)

DECLARE_CONFIG_INT	(VolumeFM,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumePSG,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeADPCM,		0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeRhythm,		0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeBD,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeSD,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeTOP,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeHH,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeTOM,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeRIM,			0,				-100,	40)



DECLARE_CONFIG_BOOL	(ShowStatusBar,		false)								// ステータスバー表示
DECLARE_CONFIG_BOOL	(ShowFDCStatus,		false)								// FDC のステータスを表示

DECLARE_CONFIG_BOOL	(UseF12AsReset,		true)								// F12 を COPY の代わりに RESET として使用
DECLARE_CONFIG_BOOL	(ShowPlacesBar,		false)								// ファイルダイアログで PLACESBAR を表示する
DECLARE_CONFIG_SYM	(JoyPortMode,		JoyPortModeType,None)				//
DECLARE_CONFIG_BOOL	(UseALTasGRPH,		false)								// ALT を GRPH に
DECLARE_CONFIG_BOOL	(UseArrowAs10key,	false)								// 方向キーをテンキーに
DECLARE_CONFIG_BOOL	(SwapPadButtons,	false)								// パッドのボタンを入れ替え
DECLARE_CONFIG_SYM	(ScreenShotName,	ScreenShotNameType, Ask)			// スクリーンショットファイル名の指定法
DECLARE_CONFIG_BOOL	(CompressSnapshot,	true)								// スナップショットを圧縮する
DECLARE_CONFIG_SYM	(KeyboardType,		KeyType,		AT106)
DECLARE_CONFIG_BOOL	(SaveDirectory,		true)								// 起動時に前回終了時のディレクトリに移動
DECLARE_CONFIG_BOOL	(AskBeforeReset,	false)								// 終了・リセット時に確認



