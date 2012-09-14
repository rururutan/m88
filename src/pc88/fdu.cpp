// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: fdu.cpp,v 1.5 1999/07/29 14:35:31 cisc Exp $

#include "headers.h"

#include "misc.h"
#include "fdc.h"
#include "fdu.h"
#include "diskmgr.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築・破棄
//
FDU::FDU()
{
	disk = 0;
	cyrinder = 0;
}

FDU::~FDU()
{
	if (disk)
		Unmount();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool FDU::Init(DiskManager* dm, int dr)
{
	diskmgr = dm;
	drive = dr;
	return true;
}

// ---------------------------------------------------------------------------
//	FDU::Mount
//	イメージを割り当てる（＝ドライブにディスクを入れる）
//
bool FDU::Mount(FloppyDisk* fd)
{
	disk = fd;
	sector = 0;

	return true;
}

// ---------------------------------------------------------------------------
//	FDU::Unmount
//
bool FDU::Unmount()
{
	disk = 0;
	return true;
}

// ---------------------------------------------------------------------------
//	FDU::SetHead
//	ヘッドの指定
//
inline void FDU::SetHead(uint hd)
{
	head = hd & 1;
	track = (cyrinder << 1) + (hd & 1);
	disk->Seek(track);
}

// ---------------------------------------------------------------------------
//	FDU::ReadID
//	セクタを一個とってくる
//
uint FDU::ReadID(uint flags, IDR* id)
{
	if (disk)
	{
		SetHead(flags);
		
		for (int i=disk->GetNumSectors(); i>0; i--)
		{
			sector = disk->GetSector();
			if (!sector)
				break;
			
			if ((flags & 0xc0) == (sector->flags & 0xc0))
			{
				*id = sector->id;
				return 0;
			}
		}
		disk->IndexHole();
	}

	return FDC::ST0_AT | FDC::ST1_MA;
}

// ---------------------------------------------------------------------------
//	FDU::Seek
//	指定されたシリンダー番号へシークする
//
//	cyrinder シーク先
//
uint FDU::Seek(uint cy)
{
	cyrinder = cy;
	return 0;
}

// ---------------------------------------------------------------------------
//	FDU::ReadSector
//	セクタを読む
//
//	head	ヘッド番号
//	id		読み込むセクタのセクタ ID
//	data	データの転送先
//
uint FDU::ReadSector(uint flags, IDR id, uint8* data)
{
	if (!disk)
		return FDC::ST0_AT | FDC::ST1_MA;
	SetHead(flags);

	int cy = -1;
	int i = disk->GetNumSectors();
	if (!i)
		return FDC::ST0_AT | FDC::ST1_MA;

	for (; i>0; i--)
	{
		IDR rid;
		if (ReadID(flags, &rid) & FDC::ST0_AT)
			return FDC::ST0_AT | FDC::ST1_ND;
		cy = rid.c;

		if (rid == id)
		{
			memcpy(data, sector->image, Min(0x2000, sector->size));

			if (sector->flags & FloppyDisk::datacrc)
				return FDC::ST0_AT | FDC::ST1_DE | FDC::ST2_DD;
			
			if (sector->flags & FloppyDisk::deleted)
				return FDC::ST2_CM;

			return 0;
		}
	}
	disk->IndexHole();
	if (cy != id.c && cy != -1)
	{
		if (cy == 0xff)
			return FDC::ST0_AT | FDC::ST1_ND | FDC::ST2_BC;
		else
			return FDC::ST0_AT | FDC::ST1_ND | FDC::ST2_NC;
	}
	return FDC::ST0_AT | FDC::ST1_ND;
}

// ---------------------------------------------------------------------------
//	FDU::WriteSector
//	セクタに書く
//
uint FDU::WriteSector(uint flags, IDR id, const uint8* data, bool deleted)
{
	if (!disk)
		return FDC::ST0_AT | FDC::ST1_MA;
	SetHead(flags);

	if (disk->IsReadOnly())
		return FDC::ST0_AT | FDC::ST1_NW;

	int i = disk->GetNumSectors();
	if (!i)
		return FDC::ST0_AT | FDC::ST1_MA;
	
	int cy = -1;
	for (; i>0; i--)
	{
		IDR rid;
		if (ReadID(flags, &rid) & FDC::ST0_AT)
			return FDC::ST0_AT | FDC::ST1_ND;
		cy = rid.c;

		if (rid == id)
		{
			uint writesize = 0x80 << Min(8, id.n);
			
			if (writesize > sector->size)
			{
				if (!disk->Resize(sector, writesize))
					return 0;
			}				
			
			memcpy(sector->image, data, writesize);
			sector->flags = (flags & 0xc0) | (deleted ? FloppyDisk::deleted : 0);
			sector->size = writesize;
			diskmgr->Modified(drive, track);
			return 0;
		}
	}
	disk->IndexHole();
	if (cy != id.c && cy != -1)
	{
		if (cy == 0xff)
			return FDC::ST0_AT | FDC::ST1_ND | FDC::ST2_BC;
		else
			return FDC::ST0_AT | FDC::ST1_ND | FDC::ST2_NC;
	}
	return FDC::ST0_AT | FDC::ST1_ND;
}

// ---------------------------------------------------------------------------
//	FDU::SenceDeviceStatus
//	デバイス・スタータスを得る
//
uint FDU::SenceDeviceStatus()
{
	uint result = 0x20 | (cyrinder ? 0 : 0x10) | (head ? 4 : 0);
	if (disk)
		result |= disk->IsReadOnly() ? 0x48 : 8;
	return result;
}

// ---------------------------------------------------------------------------
//	FDU::WriteID
//
uint FDU::WriteID(uint flags, WIDDESC* wid)
{
	int i;
	
	if (!disk)
		return FDC::ST0_AT | FDC::ST1_MA;
	if (disk->IsReadOnly())
		return FDC::ST0_AT | FDC::ST1_NW;

	SetHead(flags);

	// トラックサイズ計算
	uint sot = 0;
	uint sos = 0x80 << Min(8, wid->n);

	for (i=wid->sc; i>0; i--)
	{
		if (sot + sos + 0x40 > disk->GetTrackCapacity())
			break;
		sot += sos + 0x40;
	}
	wid->idr += i;
	wid->sc -= i;

	if (wid->sc)
	{
		if (!disk->FormatTrack(wid->sc, sos))
		{
			diskmgr->Modified(drive, track);
			return FDC::ST0_AT | FDC::ST0_EC;
		}

		for (i=wid->sc; i>0; i--)
		{
			FloppyDisk::Sector* sec = disk->GetSector();
			sec->id = *wid->idr++;
			sec->flags = flags & 0xc0;
			memset(sec->image, wid->d, sec->size);
		}
	}
	else
	{
		// unformat
		disk->FormatTrack(0, 0);
	}
	disk->IndexHole();
	diskmgr->Modified(drive, track);
	return 0;
}

// ---------------------------------------------------------------------------
//	FindID	
//
uint FDU::FindID(uint flags, IDR id)
{
	if (!disk)
		return FDC::ST0_AT | FDC::ST1_MA;

	SetHead(flags);
	
	if (!disk->FindID(id, flags & 0xc0))
	{
		FloppyDisk::Sector* sec = disk->GetSector();
		if (sec && sec->id.c != id.c)
		{
			if (sec->id.c == 0xff)
				return FDC::ST0_AT | FDC::ST1_ND | FDC::ST2_BC;
			else
				return FDC::ST0_AT | FDC::ST1_ND | FDC::ST2_NC;
		}
		disk->IndexHole();
		return FDC::ST0_AT | FDC::ST1_ND;
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	ReadDiag 用のデータ作成
//
uint FDU::MakeDiagData(uint flags, uint8* data, uint* size)
{
	if (!disk)
		return FDC::ST0_AT | FDC::ST1_MA;
	SetHead(flags);

	FloppyDisk::Sector* sec = disk->GetFirstSector(track);
	if (!sec)
		return FDC::ST0_AT | FDC::ST1_ND;
	if (sec->flags & FloppyDisk::mam)
		return FDC::ST0_AT | FDC::ST1_MA | FDC::ST2_MD;

	int capacity = disk->GetTrackCapacity();
	if ((flags & 0x40) == 0) capacity >>= 1;

	uint8* dest = data;
	uint8* limit = data + capacity;
	DiagInfo* diaginfo = (DiagInfo*) (data + 0x3800);

	// プリアンプル
	if (flags & 0x40)
	{	// MFM
		memset(dest    , 0x4e, 80);		// GAP4a
		memset(dest+ 80, 0x00, 12);		// Sync
		dest[92] = 0xc2; dest[93] = 0xc2;	// IAM
		dest[94] = 0xc2; dest[95] = 0xfc;
		memset(dest+ 96, 0x4e, 50);		// GAP1
		dest += 146, capacity -= 146;
	}
	else
	{	// FM
		memset(dest    , 0xff, 40);		// GAP4a
		memset(dest+ 40, 0x00,  6);		// Sync
		dest[46] = 0xfc;				// IAM
		memset(dest+ 47, 0xff, 50);		// GAP1
		dest += 97, capacity -= 97;
	}

	for (; sec && dest < limit; sec = sec->next)
	{
		if (((flags ^ sec->flags) & 0xc0) == 0)
		{
			// tracksize = 94/49 + secsize
			// ID
			if (flags & 0x40)
			{	// MFM
				memset(dest   , 0, 12);			// SYNC
				dest[12] = 0xa1; dest[13] = 0xa1; // IDAM
				dest[14] = 0xa1; dest[15] = 0xfe;
				dest[16] = sec->id.c; dest[17] = sec->id.h;
				dest[18] = sec->id.r; dest[19] = sec->id.n;
				memset(dest+20, 0x4e, 22+2);	// CRC+GAP2
				memset(dest+44, 0, 12);			// SYNC
				dest[56] = 0xa1; dest[57] = 0xa1; // IDAM
				dest[58] = 0xa1; dest[59] = sec->flags & FloppyDisk::deleted ? 0xfb : 0xf8;
				dest += 60;
			}
			else
			{	// FM
				memset(dest   , 0, 6);			// SYNC
				dest[ 6] = 0xfe;					// IDAM
				dest[ 7] = sec->id.c; dest[ 8] = sec->id.h;
				dest[ 9] = sec->id.r; dest[10] = sec->id.n;
				memset(dest+11, 0xff, 11+2);	// CRC+GAP2
				memset(dest+24, 0, 6);			// SYNC
				dest[30] = sec->flags & FloppyDisk::deleted ? 0xfb : 0xf8;
				dest += 31;
			}

			// DATA PART
			diaginfo->idr = sec->id;
			diaginfo->data = dest;
			diaginfo++;

			memcpy(dest, sec->image, sec->size);
			dest += sec->size;
			if (flags & 0x40)
				memset(dest, 0x4e, 2+0x20), dest+=0x22;
			else
				memset(dest, 0xff, 2+0x10), dest+=0x12;
		}
		else
		{
			if (flags & 0x40)
			{
				memset(dest, 0, (49+sec->size) * 2);
				dest += (49+sec->size) * 2;
			}
			else
			{
				memset(dest, 0, (94+sec->size) / 2);
				dest += (49+sec->size) / 2;
			}
		}
	}
	if (dest == data)
		return FDC::ST0_AT | FDC::ST1_ND;
	if (dest < limit)
	{
		memset(dest, (flags & 0x40) ? 0x4e : 0xff, limit-dest);
		dest = limit;
	}

	*size = dest - data;
	diaginfo->data = 0;
	return 0;
}

// ---------------------------------------------------------------------------
//	ReadDiag 本体
//
uint FDU::ReadDiag(uint8* data, uint8** cursor, IDR idr)
{
	uint8* dest = *cursor;
	DiagInfo* diaginfo = (DiagInfo*) (data + 0x3800);

	while (diaginfo->data && dest > diaginfo->data)
		diaginfo++;
	if (!diaginfo->data)
		diaginfo = (DiagInfo*) (data + 0x3800);

	*cursor = diaginfo->data;
	if (idr == diaginfo->idr)
		return 0;
	return FDC::ST1_ND;
}
