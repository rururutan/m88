// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	CD-ROM インターフェースの実装
// ----------------------------------------------------------------------------
//	$Id: cdif.h,v 1.2 1999/10/10 01:39:00 cisc Exp $

#pragma once

#include "device.h"
#include "cdctrl.h"
#include "cdrom.h"
#include "if/ifpc88.h"

namespace PC8801
{

class CDIF : public Device
{
public:
	enum IDIn
	{
		in90=0, in91, in92, in93, in96, in98, in99, in9b, in9d,
	};
	enum IDOut
	{
		reset=0, out90, out91, out94, out97, out98, out99, out9f,
	};

public:
	CDIF(const ID& id);
	~CDIF();
	const Descriptor* IFCALL GetDesc() const { return &descriptor; }
	bool Init(IDMAAccess* mdev);
	
	bool Enable(bool f);

	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);

	void IOCALL SystemReset(uint, uint d);
	void IOCALL Out90(uint, uint d);
	void IOCALL Out91(uint, uint d);
	void IOCALL Out94(uint, uint d);
	void IOCALL Out97(uint, uint d);
	void IOCALL Out98(uint, uint d);
	void IOCALL Out99(uint, uint d);
	void IOCALL Out9f(uint, uint d);
	uint IOCALL In90(uint);
	uint IOCALL In91(uint);
	uint IOCALL In92(uint);
	uint IOCALL In93(uint);
	uint IOCALL In96(uint);
	uint IOCALL In98(uint);
	uint IOCALL In99(uint);
	uint IOCALL In9b(uint);
	uint IOCALL In9d(uint);
	
	void Reset();

private:
	enum Phase
	{
		idlephase, cmd1phase, cmd2phase, paramphase, execphase, waitphase, 
		resultphase, statusphase, sendphase, endphase, recvphase
	};
	typedef void (CDIF::*CommandFunc)();

private:
	void DataIn();
	void DataOut();
	void ProcessCommand();
	void ResultPhase(int r, int s);
	void SendPhase(int b, int r, int s);
	void RecvPhase(int b);
	void SendCommand(uint c, uint a1=0, uint a2=0);
	void Done(int ret);

	void CheckDriveStatus();
	void ReadTOC();
	void TrackSearch();
	void ReadSubcodeQ();
	void PlayStart();
	void PlayStop();
	void SetReadMode();
	void ReadSector();

	uint GetPlayAddress();
		
	enum { ssrev = 1, };
	struct Snapshot
	{
		uint8	rev;
		uint8	phase;
		uint8	status;
		uint8	data;
		uint8	playmode;
		uint8	retrycount;
		uint8	stillmode;
		uint8	rslt;
		uint8	stat;

		uint	sector;
		uint16	ptr;
		uint16	length;
		uint	addrs;
		
		uint8	buf[16+16+2340];
	};
	
	IDMAAccess* dmac;
	
	int phase;
	
	uint8* ptr;				// 転送モード
	int length;

	uint addrs;				// 再生開始アドレス
	
	uint status;			// in 90
	uint data;				// port 91
	uint playmode;			// port 98
	uint retrycount;
	uint readmode;
	uint sector;

	uint8 stillmode;
	
	uint8 clk;
	uint8 rslt;
	uint8 stat;
	bool enable;
	bool active;

	uint8 cmdbuf[16];		// バッファは連続して配置されること
	uint8 datbuf[16];
	uint8 tmpbuf[2340];
	CDROM cdrom;
	CDControl cd;

	static const Descriptor descriptor;
	static const InFuncPtr indef[];
	static const OutFuncPtr outdef[];
};

}; // namespace PC8801

