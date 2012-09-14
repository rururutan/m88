// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: floppy.cpp,v 1.4 1999/07/29 14:35:31 cisc Exp $

#include "headers.h"
#include "floppy.h"

// ---------------------------------------------------------------------------
//	構築
//
FloppyDisk::FloppyDisk()
{
	ntracks = 0;
	curtrack = 0;
	cursector = 0;
	curtracknum = ~0;
	readonly = false;
	type = MD2D;
}

FloppyDisk::~FloppyDisk()
{
}

// ---------------------------------------------------------------------------
//	初期化
//
bool FloppyDisk::Init(DiskType _type, bool _readonly)
{
	static const int trtbl[] = { 84, 164, 164 };
	
	type = _type;
	readonly = _readonly;
	ntracks = trtbl[type];

	curtrack = 0;
	cursector = 0;
	curtracknum = ~0;
	return true;
}

// ---------------------------------------------------------------------------
//	指定のトラックにシーク
//
void FloppyDisk::Seek(uint tr)
{
	if (tr != curtracknum)
	{
		curtracknum = tr;
		curtrack = tr < 168 ? tracks + tr : 0;
		cursector = 0;
	}
}

// ---------------------------------------------------------------------------
//	セクタ一つ読み出し
//
FloppyDisk::Sector* FloppyDisk::GetSector()
{
	if (!cursector)
	{
		if (curtrack)
			cursector = curtrack->sector;
	}
	
	Sector* ret = cursector;
	
	if (cursector)
		cursector = cursector->next;
	
	return ret;
}

// ---------------------------------------------------------------------------
//	指定した ID を検索
//
bool FloppyDisk::FindID(IDR idr, uint density)
{
	if (!curtrack)
		return false;
	
	Sector* first = cursector;
	do
	{
		if (cursector)
		{
			if (cursector->id == idr)
			{
				if ((cursector->flags & 0xc0) == (density & 0xc0))
					return true;
			}
			cursector = cursector->next;
		}
		else
		{
			cursector = curtrack->sector;
		}
	} while (cursector != first);
	
	return false;
}

// ---------------------------------------------------------------------------
//	セクタ数を得る
//
uint FloppyDisk::GetNumSectors()
{
	int n = 0;
	if (curtrack)
	{
		Sector* sec = curtrack->sector;
		while (sec)
		{
			sec = sec->next;
			n++;
		}
	}
	return n;
}

// ---------------------------------------------------------------------------
//	トラック中のセクタデータの総量を得る
//
uint FloppyDisk::GetTrackSize()
{
	int size=0;

	if (curtrack)
	{
		Sector* sec = curtrack->sector;
		while (sec)
		{
			size += sec->size;
			sec = sec->next;
		}
	}
	return size;
}

// ---------------------------------------------------------------------------
//	Floppy::Resize
//	セクタのサイズを大きくした場合におけるセクタ潰しの再現
//	sector は現在選択しているトラックに属している必要がある．
//
bool FloppyDisk::Resize(Sector* sec, uint newsize)
{
	assert(curtrack && sec);

	int extend = newsize - sec->size - 0x40;
	
	// sector 自身の resize
	delete[] sec->image;
	sec->image = new uint8[newsize];
	sec->size = newsize;

	if (!sec->image)
	{
		sec->size = 0;
		return false;
	}
	
	cursector = sec->next;
	while (extend > 0 && cursector)
	{
		Sector* next = cursector->next;
		extend -= cursector->size + 0xc0;
		delete[] cursector->image;
		delete cursector;
		sec->next = cursector = next;
	}
	if (extend > 0)
	{
		int gapsize = GetTrackCapacity() - GetTrackSize() - 0x60 * GetNumSectors();
		extend -= gapsize;
	}
	while (extend > 0 && cursector)
	{
		Sector* next = cursector->next;
		extend -= cursector->size + 0xc0;
		delete[] cursector->image;
		delete cursector;
		curtrack->sector = cursector = next;
	}
	if (extend > 0)
		return false;

	return true;
}

// ---------------------------------------------------------------------------
//	FloppyDisk::FormatTrack
//
bool FloppyDisk::FormatTrack(int nsec, int secsize)
{
	Sector* sec;

	if (!curtrack)
		return false;
	
	// 今あるトラックを破棄
	sec = curtrack->sector;
	while (sec)
	{
		Sector* next = sec->next;
		delete[] sec->image;
		delete sec;
		sec = next;
	}
	curtrack->sector = 0;
	
	if (nsec)
	{
		// セクタを作成
		cursector = 0;
		for (int i=0; i<nsec; i++)
		{
			Sector* newsector = new Sector;
			if (!newsector)
				return false;
			curtrack->sector = newsector;
			newsector->next = cursector;
			newsector->size = secsize;
			if (secsize)
			{
				newsector->image = new uint8[secsize];
				if (!newsector->image)
				{
					newsector->size = 0;
					return false;
				}
			}
			else
			{
				newsector->image = 0;
			}
			cursector = newsector;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
//	セクタ一つ追加
//
FloppyDisk::Sector* FloppyDisk::AddSector(int size)
{
	if (!curtrack)
		return 0;
	
	Sector* newsector = new Sector;
	if (!newsector)
		return 0;
	if (size)
	{
		newsector->image = new uint8[size];
		if (!newsector->image)
		{
			delete newsector;
			return 0;
		}
	}
	else
	{
		newsector->image = 0;
	}
	
	if (!cursector)
		cursector = curtrack->sector;
	
	if (cursector)
	{
		newsector->next = cursector->next;
		cursector->next = newsector;
	}
	else
	{
		newsector->next = 0;
		curtrack->sector = newsector;
	}
	cursector = newsector;
	return newsector;
}

// ---------------------------------------------------------------------------
//	トラックの容量を得る
//
uint FloppyDisk::GetTrackCapacity()
{
	static const int table[3] = { 6250, 6250, 10416 }; 
	return table[type];
}

// ---------------------------------------------------------------------------
//	トラックを得る
//
FloppyDisk::Sector* FloppyDisk::GetFirstSector(uint tr)
{
	if (tr < 168)
		return tracks[tr].sector;
	return 0;
}
