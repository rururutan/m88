// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//	Sound Implemention for Win32
// ---------------------------------------------------------------------------
//	$Id: winsound.h,v 1.17 2003/05/12 22:26:36 cisc Exp $

#pragma once

#include "types.h"
#include "pc88/sound.h"
#include "sounddrv.h"
#include "critsect.h"

class PC88;
class OPNMonitor;

class SoundDumpPipe : public SoundSource
{
public:
	SoundDumpPipe();

	void SetSource(SoundSource* source) 
	{ 
		source_ = source; 
	}
	ulong GetRate()
	{
		return source_ ? source_->GetRate() : 0;
	}
	int GetChannels()
	{
		return source_ ? source_->GetChannels() : 0;
	}
	int Get(Sample* dest, int samples);
	int GetAvail()
	{
		return INT_MAX;
	}

	bool DumpStart(char* filename);
	bool DumpStop();
	bool IsDumping() { return dumpstate_ != IDLE; }

private:
	enum DumpState
	{
		IDLE, STANDBY, DUMPING
	};

	void Dump(Sample* dest, int samples);

	SoundSource* source_;
	string dumpfile_;

	HMMIO hmmio_;					// mmio handle
	MMCKINFO ckparent_;				// RIFF チャンク
	MMCKINFO ckdata_;				// data チャンク

	DumpState dumpstate_;
	int dumpedsample_;
	ulong dumprate_;

	CriticalSection cs_;

};

namespace PC8801
{
class Config;

class WinSound : public Sound
{
public:
	WinSound();
	~WinSound();

	bool Init(PC88* pc, HWND hwnd, uint rate, uint buflen);
	bool ChangeRate(uint rate, uint buflen, bool wo);
	
	void ApplyConfig(const Config* config);

	bool DumpBegin(char* filename);
	bool DumpEnd();
	bool IsDumping() { return dumper.IsDumping(); }
	
	void SetSoundMonitor(OPNMonitor* mon) { soundmon = mon; }
	
private:
	bool InitSoundBuffer(LPDIRECTSOUND lpds, uint rate);
	void Cleanup();
//	int Get(Sample* dest, int samples);

	WinSoundDriver::Driver* driver;
	
	HWND hwnd;
	uint currentrate;
	uint currentbuflen;
	uint samprate;
	
	OPNMonitor* soundmon;
	bool wodrv;
	bool useds2;

	SoundDumpPipe dumper;
};

}

