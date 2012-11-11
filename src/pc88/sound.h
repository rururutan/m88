// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//	$Id: sound.h,v 1.28 2003/05/12 22:26:35 cisc Exp $

#pragma once

#include "device.h"
#include "sndbuf2.h"
#include "srcbuf.h"
#include "lpf.h"

// ---------------------------------------------------------------------------

class PC88;
class Scheduler;

namespace PC8801
{
class Sound;
class Config;

class Sound 
: public Device, public ISoundControl, protected SoundSourceL
{
public:
	Sound();
	~Sound();
	
	bool Init(PC88* pc, uint rate, int bufsize);
	void Cleanup();	
	
	void ApplyConfig(const Config* config);
	bool SetRate(uint rate, int bufsize);

	void IOCALL UpdateCounter(uint);
	
	bool IFCALL Connect(ISoundSource* src);
	bool IFCALL Disconnect(ISoundSource* src);
	bool IFCALL Update(ISoundSource* src=0);
	int  IFCALL GetSubsampleTime(ISoundSource* src);

	void FillWhenEmpty(bool f) { soundbuf.FillWhenEmpty(f); }

	SoundSource* GetSoundSource() { return &soundbuf; }

	int		Get(Sample* dest, int size);
	int		Get(SampleL* dest, int size);
	ulong	GetRate() { return mixrate; }
	int		GetChannels() { return 2; }
	
	int		GetAvail() { return INT_MAX; }


protected:
	uint mixrate;
	uint samplingrate;		// サンプリングレート
	uint rate50;			// samplingrate / 50

private:
	struct SSNode
	{
		ISoundSource* ss;
		SSNode* next;
	};

//	SoundBuffer2 soundbuf;
	SamplingRateConverter soundbuf;

	PC88* pc;
	
	int32* mixingbuf;
	int buffersize;
	
	uint32 prevtime;
	uint32 cfgflg;
	int tdiff;
	uint mixthreshold;

	bool enabled;
	
	SSNode* sslist;
	CriticalSection cs_ss;
};

}

