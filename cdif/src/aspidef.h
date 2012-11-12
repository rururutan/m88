//	$Id: aspidef.h,v 1.2 2000/01/03 01:57:51 cisc Exp $

#pragma once

#ifdef _MSC_VER
#pragma pack(1)
#endif

#ifdef __BORLANDC__
#pragma option -a1
#endif

#define SENSE_LEN	14

//	ASPI command
enum
{
	SC_HA_INQUIRY		= 0,
	SC_GET_DEV_TYPE		= 1,
	SC_EXEC_SCSI_CMD	= 2,
	SC_ABORT_SRB		= 3,
	SC_RESET_DEV		= 4,
 	SC_GET_DISK_INFO	= 6,
	SC_RESCAN_SCSI_BUS	= 7,
};

//	ASPI command status
enum 
{
	SS_PENDING			= 0,
	SS_COMPLETE			= 1,
	SS_ABORTED			= 2,
	SS_ABORT_FAIL		= 3,
	SS_ERR				= 4,
	SS_INVALID_REQ		= 0x80,
	SS_INVALID_HA		= 0x81,
	SS_NO_DEVICE		= 0x82,
	SS_INVALID_SRB		= 0xe0,
	SS_BUFFER_ALIGN		= 0xe1,
	SS_FAILED_INIT		= 0xe4,
	SS_ASPI_IS_BUSY		= 0xe5,
	SS_BUFFER_TOO_BIG	= 0xe6,
};

// ASPI request flags
enum 
{
	SRB_POSTING					= 0x01,
	SRB_ENABLE_RESIDUAL_COUNT	= 0x04,
	SRB_DIR_IN					= 0x08,
	SRB_DIR_OUT					= 0x10,
	SRB_EVENT_NOTIFY			= 0x40,
};

// SCSI command messages
enum
{
	SCSI_TestUnitReady		= 0x00,
	SCSI_RequestSense		= 0x03,
	SCSI_Inquiry			= 0x12,
	SCSI_ModeSelect			= 0x15,
	SCSI_Copy				= 0x18,
	SCSI_ModeSense			= 0x1a,
	SCSI_StartStopUnit		= 0x1b,
	SCSI_ReceiveDiagnostic	= 0x1c,
	SCSI_SendDiagnostic		= 0x1d,
	SCSI_Compare			= 0x39,
	SCSI_CopyAndVerify		= 0x3a,
	SCSI_WriteBuffer		= 0x3b,
	SCSI_ReadBuffer			= 0x3c,
	SCSI_ChangeDefinition	= 0x40,
	SCSI_LogSelect			= 0x4c,
	SCSI_LogSense			= 0x4d,
};

// SCSI Target Status
enum
{
	TS_NO			= 0x00,
	TS_CHECK		= 0x02,
	TS_BUSY			= 0x08,
	TS_CONFLICT		= 0x18,
};

struct SRB_Header
{
	BYTE	command;
	BYTE	status;
	BYTE	haid;
	BYTE	flags;
	DWORD	rsvd;
};

struct SRB_HAInquiry
{
	BYTE	command;
	BYTE	status;
	BYTE	haid;
	BYTE	flags;
	DWORD	rsvd;
	
	BYTE	nhas;
	BYTE	hascsiid;
	BYTE	mgridstr[16];
	BYTE	haidstr[16];
	WORD	param_alignment;
	BYTE	param_report;
	BYTE	param_maxid;
	DWORD	param_maxtransfer;
	BYTE	param[8];
	WORD	reserved2;
};

struct SRB_GetDeviceBlock
{
	BYTE	command;
	BYTE	status;
	BYTE	haid;
	BYTE	flags;
	DWORD	rsvd;
	
	BYTE	targetid;
	BYTE	targetlun;
	BYTE	type;
	BYTE	rsvd2;
	BYTE	dmy[28];
};


struct SRB_ExecuteIO
{
	BYTE	command;
	BYTE	status;
	BYTE	haid;
	BYTE	flags;
	DWORD	rsvd;
	
	BYTE	targetid;
	BYTE	targetlun;
	BYTE	rsvd2[2];
	DWORD	datalen;
	BYTE*	dataptr;
	BYTE	senselen;
	BYTE	CDBlen;
	BYTE	hastatus;
	BYTE	targetstatus;
	void*	postproc;
	void*	revd3;
	BYTE	rsvd4[16];
	BYTE	CDB[16];
	BYTE	sensearea[SENSE_LEN+2];
};


struct SRB_Abort
{
	BYTE	command;
	BYTE	status;
	BYTE	haid;
	BYTE	flags;
	DWORD	rsvd;
	
	void*	service;
};


#ifdef _MSC_VER
#pragma pack()	// 3a
#endif

#ifdef __BORLANDC__
#pragma option -a.
#endif

