// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: cdrom.h,v 1.1 1999/08/26 08:04:37 cisc Exp $

#pragma once

#include "types.h"
#include "misc.h"

class ASPI;

// ---------------------------------------------------------------------------
//	CDROM 制御クラス
//
class CDROM
{
public:
	struct Track
	{
		uint32 addr;
		uint control;		// b2 = data/~audio
	};
	struct MSF
	{
		uint8 min;
		uint8 sec;
		uint8 frame;
	};

public:
	CDROM();
	~CDROM();

	int ReadTOC();
	int GetNumTracks();
	bool Init();
	bool PlayTrack(int tr, bool one = false);
	bool PlayAudio(uint begin, uint stop);
	bool ReadSubCh(uint8* dest, bool msf);
	bool Pause(bool pause);
	bool Stop();
	bool Read(uint sector, uint8* dest, int length);
	bool Read2(uint sector, uint8* dest, int length);
	bool ReadCDDA(uint sector, uint8* dest, int length);
	const Track* GetTrackInfo(int t);
	bool CheckMedia();
	MSF ToMSF(uint lba);
	uint ToLBA(MSF msf);

private:
	bool FindDrive();

	int ExecuteSCSICommand(HANDLE _hdev, void* _cdb, uint _cdblen, uint _direction = 2,
		 void* _data = 0, uint _datalen = 0);

	int m_driveletters[26];
	int m_maxcd;
	HANDLE hdev;
	int ntracks;	// トラック数
	int trstart;	// トラックの開始位置

	Track track[100];
};

// ---------------------------------------------------------------------------
//	LBA 時間を MSF 時間に変換
//
inline CDROM::MSF CDROM::ToMSF(uint lba)
{
	lba += trstart;
	MSF msf;
	msf.min = NtoBCD(lba / 75 / 60);
	msf.sec = NtoBCD(lba / 75 % 60);
	msf.frame = NtoBCD(lba % 75);
	return msf;
}

// ---------------------------------------------------------------------------
//	LBA 時間を MSF 時間に変換
//
inline uint CDROM::ToLBA(MSF msf)
{
	return (BCDtoN(msf.min) * 60 + BCDtoN(msf.sec)) * 75 + BCDtoN(msf.frame)
		   - trstart;
}

// ---------------------------------------------------------------------------
//	CD のトラック情報を取得
//	ReadTOC 後に有効
//
inline const CDROM::Track* CDROM::GetTrackInfo(int t)
{
	if (t < 0 || t > ntracks+1)
		return 0;
	return &track[t ? t - 1 : ntracks];
}

// ---------------------------------------------------------------------------
//	CD 中のトラック数を取得
//	ReadTOC 後に有効
//
inline int CDROM::GetNumTracks()
{
	return ntracks;
}

