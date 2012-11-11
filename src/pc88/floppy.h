// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: floppy.h,v 1.5 2000/02/29 12:29:52 cisc Exp $

#pragma once

#include "types.h"

// ---------------------------------------------------------------------------
//	フロッピーディスク
//
class FloppyDisk
{
public:
	enum SectorFlags
	{
		deleted		= 1,
		datacrc		= 2,
		idcrc		= 4,
		mam			= 8,
		density		= 0x40,		// MFM = 0x40, FM = 0x00
		highdensity = 0x80,		// 2HD?
	};
	enum DiskType
	{
		MD2D=0, MD2DD, MD2HD
	};
	struct IDR
	{
		uint8 c, h, r, n;

		bool operator==(const IDR& i)
		{ return ((c == i.c) && (h == i.h) && (r == i.r) && (n == i.n)); }
	};

	struct Sector
	{
		IDR id;
		uint flags;
		uint8* image;
		uint size;
		Sector* next;
	};

	class Track
	{
	public:
		Track() : sector(0) {}
		~Track()
		{
			for (Sector* s = sector; s;)
			{
				Sector* n = s->next;
				delete[] s->image;
				delete s;
				s = n;
			}
		}
		
		Sector* sector;
	};

public:
	FloppyDisk();
	~FloppyDisk();

	bool Init(DiskType type, bool readonly);
		
	bool IsReadOnly() { return readonly; }
	DiskType GetType() { return type; }
	
	void Seek(uint tr);
	Sector* GetSector();
	bool FindID(IDR idr, uint density);
	uint GetNumSectors();
	uint GetTrackCapacity();
	uint GetTrackSize();
	uint GetNumTracks() { return ntracks; }
	bool Resize(Sector* sector, uint newsize);
	bool FormatTrack(int nsec, int secsize);
	Sector* AddSector(int secsize);
	Sector* GetFirstSector(uint track);
	void IndexHole() { cursector = 0; }

private:
	Track tracks[168];
	int ntracks;
	DiskType type;
	bool readonly;

	Track* curtrack;
	Sector* cursector;
	uint curtracknum;
};

