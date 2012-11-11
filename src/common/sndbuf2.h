//	$Id: sndbuf2.h,v 1.2 2003/05/12 22:26:34 cisc Exp $

#pragma once

#include "critsect.h"
#include "if/ifcommon.h"
#include "soundsrc.h"

// ---------------------------------------------------------------------------
//	SoundBuffer2
//
class SoundBuffer2 : public SoundSource
{
public:
	SoundBuffer2();
	~SoundBuffer2();

	bool	Init(SoundSource* source, int bufsize);	// bufsize はサンプル単位
	void	Cleanup();

	int		Get(Sample* dest, int size);
	ulong	GetRate();
	int		GetChannels();

	int		Fill(int samples);			// バッファに最大 sample 分データを追加
	bool	IsEmpty();
	void	FillWhenEmpty(bool f);		// バッファが空になったら補充するか

	int		GetAvail();

private:
	int		FillMain(int samples);

	CriticalSection cs;
	
	SoundSource* source;
	Sample* buffer;
	int buffersize;						// バッファのサイズ (in samples)
	int read;							// 読込位置 (in samples)
	int write;							// 書き込み位置 (in samples)
	int ch;								// チャネル数(1sample = ch*Sample)
	bool fillwhenempty;
};

// ---------------------------------------------------------------------------

inline void SoundBuffer2::FillWhenEmpty(bool f)
{
	fillwhenempty = f;
}

inline ulong SoundBuffer2::GetRate()
{
	return source ? source->GetRate() : 0;
}

inline int SoundBuffer2::GetChannels()
{
	return source ? ch : 0;
}

inline int SoundBuffer2::GetAvail()
{
	int avail;
	if (write >= read)
		avail = write - read;
	else
		avail = buffersize + write - read;
	return avail;
}

