// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: filetest.cpp,v 1.3 1999/12/28 11:14:06 cisc Exp $

#include "headers.h"
#include "file.h"
#include "error.h"

// ---------------------------------------------------------------------------
//	自分自身の CRC をチェックする．
//	身近で PE_CIH が大発生した記念につけてみた(^^;
//
bool SanityCheck(uint32* pcrc)
{
	uint32 crc = 0;
	uint32 crctag = 0;

#ifdef _WIN32
	char buf[MAX_PATH];
	GetModuleFileName(0, buf, MAX_PATH);
	Error::SetError(Error::InsaneModule);

	FileIO fio;
	if (!fio.Open(buf, FileIO::readonly))
		return false;

	fio.Seek(0, FileIO::end);
	uint len = fio.Tellp();

	uint8* mod = new uint8[len];
	if (!mod)
		return false;

	fio.Seek(0, FileIO::begin);
	fio.Read(mod, len);
	
	const int tagpos = 0x7c;
	crctag = *(uint32*) (mod + tagpos);
	
	*(uint32*) (mod + tagpos) = 0;

	// CRC 計算
	uint crctable[256];
	uint i;
	for (i=0; i<256; i++)
	{
		uint r = i << 24;
		for (int j=0; j<8; j++)
			r = (r << 1) ^ (r & 0x80000000 ? 0x04c11db7 : 0);
		crctable[i] = r;
	}
	crc = 0xffffffff;
	for (i=0; i<len; i++)
		crc = (crc << 8) ^ crctable[((crc >> 24) ^ mod[i]) & 0xff];
	
	delete[] mod;

#endif
	if (pcrc)
		*pcrc = crc;

	crc = ~crc;
	if (crctag && crc != crctag)
		return false;
	
	return true;
}
