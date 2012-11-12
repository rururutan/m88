//	$Id: cdromdef.h,v 1.1 1999/08/26 08:04:37 cisc Exp $

#pragma once

// --------------------------------------------------------------------------
//	commands
//
#define CD_READ_CAPACITY	0x25

#define CD_READ_SUBCH		0x42
#define CD_READ_TOC			0x43
#define CD_READ_HEADER		0x44
#define CD_PLAY_AUDIO		0xa5		// long
#define CD_PLAY_AUDIO_MSF	0x47
#define CD_PLAY_AUDIO_TR	0x48
#define CD_PLAY_AUDIO_REL	0xa9		// long
#define CD_PAUSE_RESUME		0x4b
#define CD_READ_MSF			0xb9
#define CD_READ				0xbe

// --------------------------------------------------------------------------
//	CDB
//
struct CDB_ReadTOC
{
	BYTE id;
	BYTE flags;
	BYTE rsvd[4];
	BYTE track;
	WORDBE length;
	BYTE control;
};

struct CDB_PlayAudio
{
	BYTE id;
	BYTE flags;
	LONGBE start;
	LONGBE length;
	BYTE rsvd;
	BYTE control;
};

struct CDB_ReadSubChannel
{
	BYTE id;
	BYTE flag1;
	BYTE flag2;
	BYTE format;
	BYTE rsvd[2];
	BYTE track;
	WORDBE length;
	BYTE control;
};

struct CDB_PauseResume
{
	BYTE id;
	BYTE rsvd[7];
	BYTE flag;
	BYTE control;
};

struct CDB_StartStopUnit
{
	BYTE id;
	BYTE flag1;
	BYTE rsvd[2];
	BYTE flag2;
	BYTE contol;
};

struct CDB_ReadCD
{
	BYTE id;
	BYTE flag1;
	LONGBE addr;
	TRIBE length;
	BYTE flag2;
	BYTE subch;
	BYTE control;
};

struct CDB_Read
{
	BYTE id;
	BYTE flag;
	LONGBE addr;
	BYTE rsvd;
	WORDBE blocks;
	BYTE control;
};

struct CDB_TestUnitReady
{
	BYTE id;
	BYTE rsvd[4];
	BYTE control;
};


// --------------------------------------------------------------------------
//	ç\ë¢ëÃ
//
struct TOCHeader
{
	WORDBE length;
	BYTE start;
	BYTE end;
};

struct TOCEntry
{
	BYTE rsvd;
	BYTE control;
	BYTE track;
	BYTE rsvd2;
	LONGBE addr;
};

template<int t>
struct TOC
{
	TOCHeader header;
	TOCEntry entry[t];
};

struct SubcodeQ
{
	BYTE	rsvd;
	BYTE	audiostatus;
	WORDBE	length;
	BYTE	format;
	BYTE	control;
	BYTE	track;
	BYTE	index;
	LONGBE	absaddr;
	LONGBE	reladdr;
};

