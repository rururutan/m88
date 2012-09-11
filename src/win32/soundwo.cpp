// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	waveOut based driver
// ---------------------------------------------------------------------------
//	$Id: soundwo.cpp,v 1.7 2002/05/31 09:45:21 cisc Exp $

#include "headers.h"
#include "soundwo.h"
#include "status.h"

using namespace WinSoundDriver;

// ---------------------------------------------------------------------------
//	構築・破棄
//
DriverWO::DriverWO()
{
	src = 0;
	playing = false;
	hthread = 0;
	hwo = 0;
	mixalways = false;
	wavehdr = 0;
	numblocks = 4;
}

DriverWO::~DriverWO()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//  初期化 
//	s			PCM のソースとなる SoundBuffer へのポインタ
//	rate		再生周波数
//	ch			チャネル数(2 以外は未テスト)
//	buflen		バッファ長(単位: ms)
//
bool DriverWO::Init(SoundSource* s, HWND, uint rate, uint ch, uint buflen)
{
	int i;
	
	if (playing)
		return false;

	src = s;
	sampleshift = 1 + (ch == 2 ? 1 : 0);

	DeleteBuffers();

	// バッファ作成
	buffersize = (rate * ch * sizeof(Sample) * buflen / 1000 / 4) & ~7;
	wavehdr = new WAVEHDR[numblocks];
	if (!wavehdr)
		return false;

	memset(wavehdr, 0, sizeof(wavehdr) * numblocks);
	for (i=0; i<numblocks; i++)
	{
		wavehdr[i].lpData = new char[buffersize];
		if (!wavehdr[i].lpData)
		{
			DeleteBuffers();
			return false;
		}
		memset(wavehdr[i].lpData, 0, buffersize);
		wavehdr[i].dwBufferLength = buffersize;
	}

	// スレッド起動
	if (!hthread)
	{
		hthread = HANDLE(_beginthreadex(NULL, 0, ThreadEntry,
							reinterpret_cast<void*>(this), 0, &idthread));
		if (!hthread)
		{
			DeleteBuffers();
			return false;
		}
		SetThreadPriority(hthread, THREAD_PRIORITY_ABOVE_NORMAL);
	}

	// 再生フォーマット設定
	WAVEFORMATEX wf;
    memset(&wf, 0, sizeof(WAVEFORMATEX));
    wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = ch;
    wf.nSamplesPerSec = rate;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	if (waveOutOpen(&hwo, WAVE_MAPPER, &wf, idthread, 
					reinterpret_cast<DWORD>(this), CALLBACK_THREAD) 
			!= MMSYSERR_NOERROR)
	{
		hwo = 0;
		DeleteBuffers();
		return false;
	}
	
	playing = true;
	dontmix = true;

	// wavehdr の準備
	for (i=0; i<numblocks; i++)
		SendBlock(&wavehdr[i]);
	
	dontmix = false;
	return true;
}

// ---------------------------------------------------------------------------
//  後片付け
//
bool DriverWO::Cleanup()
{
	if (hthread)
	{
		PostThreadMessage(idthread, WM_QUIT, 0, 0);
		if (WAIT_TIMEOUT == WaitForSingleObject(hthread, 3000))
			TerminateThread(hthread, 0);
		CloseHandle(hthread);
		hthread = 0;
	}
	if (playing)
	{
		playing = false;
		if (hwo)
		{
			while (waveOutReset(hwo) == MMSYSERR_HANDLEBUSY)
				Sleep(10);
			for (int i=0; i<numblocks; i++)
			{
				if (wavehdr[i].dwFlags & WHDR_PREPARED)
					waveOutUnprepareHeader(hwo, wavehdr+i, sizeof(WAVEHDR));
			}
			while (waveOutClose(hwo) == MMSYSERR_HANDLEBUSY)
				Sleep(10);
			hwo = 0;
		}
	}
	DeleteBuffers();
	return true;
}

// ---------------------------------------------------------------------------
//  バッファを削除
//
void DriverWO::DeleteBuffers()
{
	if (wavehdr)
	{
		for (int i=0; i<numblocks; i++)
			delete[] wavehdr[i].lpData;
		delete[] wavehdr;
		wavehdr = 0;
	}
}

// ---------------------------------------------------------------------------
//  ブロックを 1 つ送る
//	whdr		送るブロック
//
//	dontmix	== true なら無音データを送る
//
bool DriverWO::SendBlock(WAVEHDR* whdr)
{
	if (playing)
	{
		whdr->dwUser = 0;
		whdr->dwFlags = 0;
		whdr->dwLoops = 0;
		whdr->lpNext = NULL;
		whdr->reserved = 0;

		if (!dontmix)// && (mixalways || !src->IsEmpty()))
			src->Get((Sample*) whdr->lpData, buffersize >> sampleshift);
		else
			memset(whdr->lpData, 0, buffersize);
				
		if (!waveOutPrepareHeader(hwo, whdr, sizeof(WAVEHDR)))
		{
			if (!waveOutWrite(hwo, whdr, sizeof(WAVEHDR)))
				return true;
			
			// 失敗
			waveOutUnprepareHeader(hwo, whdr, sizeof(WAVEHDR));
		}
		whdr->dwFlags = 0;
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
//  ブロック再生後の処理
//	whdr		再生が終わったブロック
//	
void DriverWO::BlockDone(WAVEHDR* whdr)
{
	if (whdr)
	{
		waveOutUnprepareHeader(hwo, whdr, sizeof(WAVEHDR));
		whdr->dwFlags = 0;

		// ブロックを送る．2 回試す
		if (!SendBlock(whdr))
			SendBlock(whdr);
	}
}

// ---------------------------------------------------------------------------
//  スレッド
//	再生が終わったブロックを送り直すだけ
//
uint __stdcall DriverWO::ThreadEntry(LPVOID arg)
{
	DriverWO* dw = reinterpret_cast<DriverWO*>(arg);
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		switch(msg.message)
		{
		case MM_WOM_DONE:
			dw->BlockDone((WAVEHDR*) msg.lParam);
			break;
		}
	}
	return 0;
}
