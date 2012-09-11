// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: aspi.cpp,v 1.1 1999/08/26 08:04:36 cisc Exp $

#include "headers.h"
#include "aspi.h"
#include "aspidef.h"

// --------------------------------------------------------------------------
//	構築
//
ASPI::ASPI()
{
	hmod = (HMODULE) LoadLibrary("wnaspi32.dll");
	
	nhostadapters = 0;
	if (ConnectAPI())
	{
		uint s = (*pgasi)();
		if ((s & 0xff00) == 0x0100)
		{
			nhostadapters = s & 0xff;
		}
	}
}

// --------------------------------------------------------------------------
//	破棄
//
ASPI::~ASPI()
{
	if (hmod)
		FreeLibrary(hmod);
}

// --------------------------------------------------------------------------
//	SendASPI32Command
//
inline uint32 ASPI::SendCommand(void* srb)
{
	return (*psac)(srb);
}

// --------------------------------------------------------------------------
//
inline uint32 ASPI::SendCommandAndWait(void* srb)
{
	uint32 result = SendCommand(srb); 
	
	volatile SRB_Header* h = (SRB_Header*) srb;
	while (h->status == SS_PENDING)
		Sleep(2);
	
	return h->status;
}

// --------------------------------------------------------------------------
//	wnaspi32.dll をインポート
//
bool ASPI::ConnectAPI()
{
	psac = 0, pgasi = 0;
	HMODULE hmod = GetModuleHandle("wnaspi32.dll");
	if (!hmod)
	{
		return false;
	}

	pgasi = (uint32 (__cdecl *)()) (GetProcAddress(hmod, "GetASPI32SupportInfo"));
	psac  = (uint32 (__cdecl *)(void*)) (GetProcAddress(hmod, "SendASPI32Command"));

	return pgasi && psac;
}

// --------------------------------------------------------------------------
//	HA_INQUIRY test
//
bool ASPI::InquiryAdapter(uint ha, uint* maxid, uint* maxxfer)
{
	if (ha < nhostadapters)
	{
		SRB_HAInquiry srb;
		memset(&srb, 0, sizeof(srb));

		srb.command = SC_HA_INQUIRY;
		srb.haid = ha;

		if (SS_COMPLETE == SendCommandAndWait(&srb))
		{
			if (maxid) 
				*maxid = srb.param_maxid ? srb.param_maxid : 8;
			if (maxxfer) 
				*maxxfer = srb.param_maxtransfer;
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------
//	SC_GET_DEV_TYPE
//
int ASPI::GetDeviceType(uint ha, uint id, uint lun)
{
	SRB_GetDeviceBlock srb;
	memset(&srb, 0, sizeof(srb));
	
	srb.command = SC_GET_DEV_TYPE;
	srb.haid = ha;
	srb.targetid = id;
	srb.targetlun = lun;
	
	if (SS_COMPLETE == SendCommandAndWait(&srb))
		return srb.type;
	else
		return -1;
}

// --------------------------------------------------------------------------
//	SC_EXEC_SCSI_CMD
//
int ASPI::ExecuteSCSICommand
(uint ha, uint id, uint lun, void* cdb, uint cdblen, uint dir, void* data, uint datalen)
{
	SRB_ExecuteIO srb;
	memset(&srb, 0, sizeof(srb));
	
	HANDLE hevent = CreateEvent(0, false, false, 0);

	srb.command = SC_EXEC_SCSI_CMD;
	srb.haid = ha;
	srb.targetid = id;
	srb.targetlun = lun;
	srb.flags = dir | SRB_EVENT_NOTIFY;
	srb.postproc = (void*) hevent;

	srb.senselen = SENSE_LEN;
	if (dir & (SRB_DIR_IN | SRB_DIR_OUT))
	{
		srb.dataptr = (uint8*) data;
		srb.datalen = datalen;
	}
	srb.CDBlen = cdblen;
	memcpy(srb.CDB, cdb, cdblen);

	int r = SendCommand(&srb);
	if (r == SS_PENDING)
	{
		if (WAIT_TIMEOUT == WaitForSingleObject(hevent, 10000))
			AbortService(ha, &srb);
		r = srb.status;
	}
	CloseHandle(hevent);

	if (r == SS_COMPLETE)
	{
		if (srb.hastatus != 0)
			return ~0x1ff | srb.hastatus;
		return srb.targetstatus;
	}
	else
	{
		return ~0xff | srb.targetstatus;
	}
}

// --------------------------------------------------------------------------
//	SC_ABORT_SRB
//
void ASPI::AbortService(uint ha, void* asrb)
{
	SRB_Abort srb;
	memset(&srb, 0, sizeof(srb));

	srb.command = SC_ABORT_SRB;
	srb.haid = ha;
	srb.service = asrb;
	SendCommand(&srb);
}
