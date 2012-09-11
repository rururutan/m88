// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//	$Id: winsound.cpp,v 1.27 2003/05/12 22:26:36 cisc Exp $

#include "headers.h"
#include "WinSound.h"
	#include "misc.h"
#include "pc88/config.h"
#include "status.h"
#include "soundds.h"
#include "soundds2.h"
#include "soundwo.h"
#include "soundmon.h"

//#define LOGNAME "winsound"
#include "diag.h"

using namespace PC8801;
using namespace WinSoundDriver;

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinSound::WinSound()
: driver(0)
{
	soundmon = 0;
	useds2 = true;
}

WinSound::~WinSound()
{
	DumpEnd();
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinSound::Init
(PC88* pc, HWND hwindow, uint rate, uint buflen)
{
	currentrate = 100;
	currentbuflen = 0;
	hwnd = hwindow;

	dumper.SetSource(GetSoundSource());
	
	if (!Sound::Init(pc, 8000, 0))
		return false;
	return true;
}

// ---------------------------------------------------------------------------
//	後処理
//
void WinSound::Cleanup()
{
	if (driver)
	{
		driver->Cleanup();
		delete driver;
		driver = 0;
	}
	Sound::Cleanup();
}

// ---------------------------------------------------------------------------
//	合成・再生レート変更
//
bool WinSound::ChangeRate(uint rate, uint buflen, bool waveout)
{
	if (currentrate != rate || currentbuflen != buflen || wodrv != waveout)
	{
		if (IsDumping())
		{
			statusdisplay.Show(70, 3000, "wav 書き出し時の音設定の変更はできません");
			return false;
		}

		samprate = rate;
		currentrate = rate;
		currentbuflen = buflen;
		wodrv = waveout;
		
		if (rate < 8000)
		{
			rate = 100; samprate = 0;
		}
		
		// DirectSound: サンプリングレート * バッファ長 / 2
		// waveOut:     サンプリングレート * バッファ長 * 2
		int bufsize;
		if (wodrv)
			bufsize = (samprate * buflen / 1000 * 1) & ~15;
		else
			bufsize = (samprate * buflen / 1000 / 2) & ~15;
		
		if (driver)
		{
			driver->Cleanup();
			delete driver;
			driver = 0;
		}

		if (rate < 1000)
			bufsize = 0;
			
		if (!SetRate(rate, bufsize))
			return false;
		
		if (bufsize > 0)
		{
			for (int i=0; i<1; i++)
			{
				if (wodrv)
				{
					driver = new DriverWO;
				}
				else if (useds2)
				{
					driver = new DriverDS2;
					statusdisplay.Show(200, 8000, "sounddrv: using Notify driven driver");
				}
				else
				{
					driver = new DriverDS;
					statusdisplay.Show(200, 8000, "sounddrv: using timer driven driver");
				}
				
				if (!driver || !driver->Init(&dumper, hwnd, samprate, 2, buflen))
				{
					delete driver; driver = 0;
					if (!wodrv && useds2)
					{
						useds2 = false;
						statusdisplay.Show(100, 3000, "IDirectSoundNotify は使用できないようです");
						i = -1;
					}
				}
			}
			if (!driver)
			{
				SetRate(rate, 0);
				statusdisplay.Show(70, 3000, "オーディオデバイスを使用できません");
			}
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
//	設定更新
//
void WinSound::ApplyConfig(const Config* config)
{
	useds2 = !!(config->flag2 & Config::usedsnotify);


	bool wo = (config->flag2 & Config::usewaveoutdrv) != 0;
	ChangeRate(config->sound, config->soundbuffer, wo);
	
	if (driver)
		driver->MixAlways(0 != (config->flags & Config::mixsoundalways));
	
	Sound::ApplyConfig(config);
}


bool WinSound::DumpBegin(char* filename)
{
	if (!dumper.DumpStart(filename))
		return false;

//	FillWhenEmpty(false);
	return true;
}

bool WinSound::DumpEnd()
{
	if (!dumper.DumpStop())
		return false;

//	FillWhenEmpty(true);
	return true;
}

// ---------------------------------------------------------------------------
//	合成の場合
//
//int WinSound::Get(Sample* dest, int samples)
//{
//	return samples;
//}


SoundDumpPipe::SoundDumpPipe()
	: source_(0)
	, dumpstate_(IDLE)
	, hmmio_(0)
{
}

bool SoundDumpPipe::DumpStart(char* filename)
{
	if (hmmio_)
		return false;

	TCHAR path[MAX_PATH];
	LPTSTR filepart;
	GetFullPathName(filename, sizeof(path), path, &filepart);
	dumpfile_ = path;

	memset(&ckparent_, 0, sizeof(MMCKINFO));
	memset(&ckdata_, 0, sizeof(MMCKINFO));

	// mmioOpen
	hmmio_ = mmioOpen(filename, NULL, MMIO_CREATE | MMIO_WRITE | MMIO_ALLOCBUF); 
	if (!hmmio_)
		return false;
	
	// WAVE chunk
	ckparent_.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	if (mmioCreateChunk(hmmio_, &ckparent_, MMIO_CREATERIFF))
	{
		mmioClose(hmmio_, 0);
		hmmio_ = 0;
		return false;
	}

	// fmt chunk
	MMCKINFO cksub;
	memset(&cksub, 0, sizeof(MMCKINFO));
	cksub.ckid = mmioFOURCC('f', 'm', 't', ' ');
	mmioCreateChunk(hmmio_, &cksub, 0);
	
	dumprate_ = GetRate();

	WAVEFORMATEX format;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = GetChannels();
	format.nSamplesPerSec = dumprate_;
	format.wBitsPerSample = 16;
	format.nAvgBytesPerSec = format.nChannels * format.nSamplesPerSec * 2;
	format.nBlockAlign = format.nChannels * 2;
	format.cbSize = 0;

	mmioWrite(hmmio_, HPSTR(&format), sizeof(format));
	mmioAscend(hmmio_, &cksub, 0);

	// data chunk
	ckdata_.ckid = mmioFOURCC('d', 'a', 't', 'a');
	mmioCreateChunk(hmmio_, &ckdata_, 0);
	
	dumpstate_ = STANDBY;
	dumpedsample_ = 0;
	statusdisplay.Show(100, 0, "録音待機中～");
	return true;
}

// ---------------------------------------------------------------------------
//	ダンプ終了
//
bool SoundDumpPipe::DumpStop()
{
	if (dumpstate_ != IDLE)
	{
		dumpstate_ = IDLE;
		CriticalSection::Lock lock(cs_);

		if (ckdata_.dwFlags & MMIO_DIRTY) 
			mmioAscend(hmmio_, &ckdata_, 0);
		if (ckparent_.dwFlags & MMIO_DIRTY)
			mmioAscend(hmmio_, &ckparent_, 0);
		if (hmmio_)
			mmioClose(hmmio_, 0), hmmio_ = 0;

		int curtime = dumpedsample_ / dumprate_;
		statusdisplay.Show
			(100, 2500, "録音終了 %s [%.2d:%.2d]", dumpfile_.c_str(), curtime/60, curtime%60);
	}
	return true;
}


int SoundDumpPipe::Get(Sample* dest, int samples)
{
	if (!source_)
		return 0;

	if (dumpstate_ == IDLE)
		return source_->Get(dest, samples);

	int avail = source_->GetAvail();
	
	int actual_samples = source_->Get(dest, Min(avail, samples));

	int nch = GetChannels();
	fill(dest + actual_samples * nch, dest + samples * nch, 0);

	CriticalSection::Lock lock(cs_);
	if (dumpstate_ != IDLE)
	{
		Dump(dest, actual_samples);
	}

	return actual_samples;
}

void SoundDumpPipe::Dump(Sample* dest, int samples)
{
	int nch = GetChannels();

	// 冒頭の無音部をカットする
	if (dumpstate_ == STANDBY)
	{
		int i;
		uint32* s = (uint32*) dest;
		for (i=0; i<samples && *s == 0; i++, s++)
			;
		dest += i * nch, samples -= i;
		if (samples > 0)
		{
			dumpstate_ = DUMPING;
		}
	}

	if (samples)
	{
		mmioWrite(hmmio_, (char*) dest, samples * sizeof(Sample) * nch);

		// 録音時間表示
		int prevtime = dumpedsample_ / dumprate_;
		dumpedsample_ += samples;
		int curtime = dumpedsample_ / dumprate_;
		if (prevtime != curtime)
		{
			statusdisplay.Show
				(101, 0, "録音中 %s [%.2d:%.2d]", dumpfile_.c_str(), curtime / 60, curtime % 60);
		}	
	}
}
