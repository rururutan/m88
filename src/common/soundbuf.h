// ---------------------------------------------------------------------------
//	class SoundBuffer 
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: soundbuf.h,v 1.7 2002/04/07 05:40:08 cisc Exp $

#pragma once

#include "types.h"
#include "critsect.h"
#include "if/ifcommon.h"

// ---------------------------------------------------------------------------
//	SoundBuffer
//
class SoundBuffer
{
public:
	typedef int16 Sample;

public:
	SoundBuffer();
	~SoundBuffer();

	bool Init(int nch, int bufsize);	// bufsize はサンプル単位
	void Cleanup();

	void Put(int sample);				// バッファに最大 sample 分データを追加
	void Get(Sample* ptr, int sample);	// バッファから sample 分のデータを得る
	bool IsEmpty();
	void FillWhenEmpty(bool f);			// バッファが空になったら補充するか

private:
	virtual void Mix(Sample* b1, int s1, Sample* b2=0, int s2=0) = 0;	// sample 分のデータ生成
	void PutMain(int sample);
	
	Sample* buffer;
	CriticalSection cs;
	
	int buffersize;						// バッファのサイズ (in samples)
	int read;							// 読込位置 (in samples)
	int write;							// 書き込み位置 (in samples)
	int ch;								// チャネル数(1sample = ch*Sample)
	bool fillwhenempty;
};

// ---------------------------------------------------------------------------

inline void SoundBuffer::FillWhenEmpty(bool f)
{
	fillwhenempty = f;
}

