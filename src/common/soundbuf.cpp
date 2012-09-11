// ---------------------------------------------------------------------------
//	class SoundBuffer 
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: soundbuf.cpp,v 1.5 1999/11/26 10:13:25 cisc Exp $

#include "headers.h"
#include "soundbuf.h"
#include "misc.h"

// ---------------------------------------------------------------------------
//	Sound Buffer
//
SoundBuffer::SoundBuffer()
: buffer(0), buffersize(0), ch(0)
{
	fillwhenempty = true;
}

SoundBuffer::~SoundBuffer()
{
	Cleanup();
}

bool SoundBuffer::Init(int nch, int bufsize)
{
	CriticalSection::Lock lock(cs);
	
	delete[] buffer; buffer = 0;
	
	buffersize = bufsize;
	ch = nch;
	read = 0; write = 0;

	if (ch && buffersize > 0)
	{
		buffer = new Sample[ch * buffersize];
		if (!buffer)
			return false;

		memset(buffer, 0, ch * buffersize * sizeof(Sample));
	}
	return true;
}

void SoundBuffer::Cleanup()
{
	CriticalSection::Lock lock(cs);
	
	delete[] buffer; buffer = 0;
}

// ---------------------------------------------------------------------------
//	バッファに音を追加
//
void SoundBuffer::Put(int samples)
{
	CriticalSection::Lock lock(cs);
	if (buffer)
		PutMain(samples);
}

void SoundBuffer::PutMain(int samples)
{
	// リングバッファの空きを計算
	int free;
	if (read <= write)
		free = buffersize + read - write;
	else
		free = read - write;

	if (!fillwhenempty && (samples > free-1))
	{
		int skip = Min(samples-free+1, buffersize-free);
		free += skip;
		read += skip;
		if (read > buffersize)
			read -= buffersize;
	}
	
	// 書きこむべきデータ量を計算
	samples = Min(samples, free-1);
	if (samples > 0)
	{
		// 書きこむ
		if (buffersize - write >= samples)
		{
			// 一度で書ける場合
			Mix(buffer + write * ch, samples);
		}
		else
		{
			// ２度に分けて書く場合
			Mix(buffer + write * ch, buffersize - write, buffer, samples - (buffersize - write));
		}
		write += samples;
		if (write >= buffersize)
			write -= buffersize;
	}
}

// ---------------------------------------------------------------------------
//	バッファから音を貰う
//
void SoundBuffer::Get(Sample* dest, int samples)
{
	CriticalSection::Lock lock(cs);
	if (buffer)
	{
		while (samples > 0)
		{
			int xsize = Min(samples, buffersize - read);
			
			int avail;
			if (write >= read)
				avail = write - read;
			else
				avail = buffersize + write - read;

			// 供給不足なら追加
			if (xsize <= avail || fillwhenempty)
			{
				if (xsize > avail)
					PutMain(xsize - avail);
				memcpy(dest, buffer + read * ch, xsize * ch * sizeof(Sample));
				dest += xsize * ch;
				read += xsize;
			}
			else
			{
				if (avail > 0)
				{
					memcpy(dest, buffer + read * ch, avail * ch * sizeof(Sample));
					dest += avail * ch;
					read += avail;
				}
				memset(dest, 0, (xsize - avail) * ch * sizeof(Sample));
				dest += (xsize - avail) * ch;
			}
			
			samples -= xsize;
			if (read >= buffersize)
				read -= buffersize;
		}
	}
}

// ---------------------------------------------------------------------------
//	バッファが空か，空に近い状態か?
//
bool SoundBuffer::IsEmpty()
{
	int avail;
	if (write >= read)
		avail = write - read;
	else
		avail = buffersize + write - read;
	
	return avail == 0;
}

