// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	waveOut based driver
// ---------------------------------------------------------------------------
//	$Id: soundwo.h,v 1.5 2002/05/31 09:45:21 cisc Exp $

#pragma once

#include "sounddrv.h"
#include "critsect.h"

// ---------------------------------------------------------------------------

namespace WinSoundDriver
{

class DriverWO : public Driver
{
public:
	DriverWO();
	~DriverWO();

	bool Init(SoundSource* sb, HWND hwnd, uint rate, uint ch, uint buflen);
	bool Cleanup();

private:
	bool SendBlock(WAVEHDR*);
	void BlockDone(WAVEHDR*);
	static uint __stdcall ThreadEntry(LPVOID arg);
	static void CALLBACK WOProc(HWAVEOUT hwo, uint umsg, DWORD inst, DWORD p1, DWORD p2);
	void DeleteBuffers();
	
	HWAVEOUT hwo;
	HANDLE hthread;
	uint idthread;
	int numblocks;					// WAVEHDR(PCM ブロック)の数
	WAVEHDR* wavehdr;				// WAVEHDR の配列
	bool dontmix;					// WAVE を送る際に音声の合成をしない
};

}

