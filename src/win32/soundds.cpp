// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1999, 2000.
// ---------------------------------------------------------------------------
//	DirectSound based driver
// ---------------------------------------------------------------------------
//	$Id: soundds.cpp,v 1.10 2002/05/31 09:45:21 cisc Exp $

#include "headers.h"
#include "soundds.h"

//#define LOGNAME "soundds"
#include "diag.h"

using namespace WinSoundDriver;

// ---------------------------------------------------------------------------

const uint DriverDS::num_blocks = 5;
const uint DriverDS::timer_resolution = 20;

// ---------------------------------------------------------------------------
//	構築・破棄 ---------------------------------------------------------------

DriverDS::DriverDS()
{
	playing = false;
	mixalways = false;
	lpds = 0;
	lpdsb = 0;
	lpdsb_primary = 0;
	timerid = 0;
	sending = false;
}

DriverDS::~DriverDS()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//  初期化 -------------------------------------------------------------------

bool DriverDS::Init(SoundSource* s, HWND hwnd, uint rate, uint ch, uint buflen)
{
	if (playing)
		return false;

	src = s;
	buffer_length = buflen;
	sampleshift = 1 + (ch == 2 ? 1 : 0);

	// 計算
	buffersize = (rate * ch * sizeof(Sample) * buffer_length / 1000) & ~7;

	// DirectSound object 作成
	if (FAILED(CoCreateInstance(CLSID_DirectSound, 0, CLSCTX_ALL, IID_IDirectSound, (void**) &lpds)))
		return false;
	if (FAILED(lpds->Initialize(0)))
		return false;
//	if (FAILED(DirectSoundCreate(0, &lpds, 0)))
//		return false;

	// 協調レベル設定
	if (DS_OK != lpds->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))
	{
		if (DS_OK != lpds->SetCooperativeLevel(hwnd, DSSCL_NORMAL))
			return false;
	}
	
	DSBUFFERDESC dsbd;
    memset(&dsbd, 0, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0; 
    dsbd.lpwfxFormat = 0;
	if (DS_OK != lpds->CreateSoundBuffer(&dsbd, &lpdsb_primary, 0))
		return false;

	// 再生フォーマット設定
	WAVEFORMATEX wf;
    memset(&wf, 0, sizeof(WAVEFORMATEX));
    wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = ch;
    wf.nSamplesPerSec = rate;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	lpdsb_primary->SetFormat(&wf);

	// セカンダリバッファ作成
    memset(&dsbd, 0, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_STICKYFOCUS 
    			 | DSBCAPS_GETCURRENTPOSITION2;

    dsbd.dwBufferBytes = buffersize; 
    dsbd.lpwfxFormat = &wf;
    
	HRESULT res = lpds->CreateSoundBuffer(&dsbd, &lpdsb, NULL); 
	if (DS_OK != res)
		return false;

	// 再生
	lpdsb->Play(0, 0, DSBPLAY_LOOPING);
//	lpdsb_primary->Play(0, 0, DSBPLAY_LOOPING);

	// タイマー作成
	timeBeginPeriod(buffer_length / num_blocks);
	timerid = timeSetEvent(buffer_length / num_blocks, timer_resolution,
		TimeProc, reinterpret_cast<DWORD>(this), TIME_PERIODIC);
	nextwrite = 1 << sampleshift;
	
	if (!timerid)
	{
		timeEndPeriod(buffer_length / num_blocks);
		return false;
	}

	playing = true;
	return true;
}

// ---------------------------------------------------------------------------
//  後片付け -----------------------------------------------------------------

bool DriverDS::Cleanup()
{
	playing = false;
	if (timerid)
	{
		timeKillEvent(timerid);
		timeEndPeriod(buffer_length / num_blocks);
		timerid = 0;
	}
	for (int i=0; i<300 && sending; i++)
		Sleep(10);
	if (lpdsb)
	{
		lpdsb->Stop();
		lpdsb->Release(), lpdsb = 0;
	}
	if (lpdsb_primary)
		lpdsb_primary->Release(), lpdsb_primary = 0;
	if (lpds)
		lpds->Release(), lpds = 0;
	return true;
}

// ---------------------------------------------------------------------------
//  TimeProc  ----------------------------------------------------------------

void CALLBACK DriverDS::TimeProc(UINT uid, UINT, DWORD_PTR user, DWORD_PTR, DWORD_PTR)
{
	DriverDS* inst = reinterpret_cast<DriverDS*>(user);
	if (inst)
		inst->Send();
}

// ---------------------------------------------------------------------------
//  ブロック送る -------------------------------------------------------------

void DriverDS::Send()
{
	if (playing && !InterlockedExchange(&sending, true))
	{
		bool restored = false;

		// Buffer Lost ?
		DWORD status;
		lpdsb->GetStatus(&status);
		if (DSBSTATUS_BUFFERLOST & status)
		{
			// restore the buffer
//			lpdsb_primary->Restore();
			if (DS_OK != lpdsb->Restore())
				goto ret;
			nextwrite = 0;
			restored = true;
		}

		// 位置取得
		DWORD cplay, cwrite;
		lpdsb->GetCurrentPosition(&cplay, &cwrite);

		if (cplay == nextwrite && !restored)
			goto ret;

		// 書きこみサイズ計算
		int writelength;
		if (cplay < nextwrite)
			writelength = cplay + buffersize - nextwrite;
		else
			writelength = cplay - nextwrite;

		writelength &= -1 << sampleshift;

		if (writelength <= 0)
			goto ret;

		LOG3("play = %5d  write = %5d  length = %5d\n", cplay, nextwrite, writelength);
		{
			void* a1, * a2;
			DWORD al1, al2;
			// Lock
			if (DS_OK != lpdsb->Lock(nextwrite, writelength,
									 (void**) &a1, &al1, (void**) &a2, &al2, 0))
				goto ret;

			// 書きこみ
			
	//		if (mixalways || !src->IsEmpty())
			{
				if (a1)
					src->Get((Sample*) a1, al1 >> sampleshift);
				if (a2)
					src->Get((Sample*) a2, al2 >> sampleshift);
			}
			
			// Unlock
			lpdsb->Unlock(a1, al1, a2, al2);
		}

		nextwrite += writelength;
		if (nextwrite >= buffersize)
			nextwrite -= buffersize;

		if (restored)
			lpdsb->Play(0, 0, DSBPLAY_LOOPING);

		// 終了
ret:
		InterlockedExchange(&sending, false);
	}
	return;
}
