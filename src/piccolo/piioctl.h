#pragma once

//
// Define the IOCTL codes we will use.  The IOCTL code contains a command
// identifier, plus other information about the device, the type of access
// with which the file must have been opened, and the type of buffering.
//

//
// Device type           -- in the "User Defined" range."
//

#define ROMEO_TYPE 40000

// The IOCTL function codes from 0x800 to 0xFFF are for customer use.

#define IOCTL_ROMEO_OPN3_WRITE_UCHAR \
    CTL_CODE( ROMEO_TYPE, 0x900, METHOD_BUFFERED, FILE_WRITE_ACCESS )

#define IOCTL_ROMEO_OPN3_READ_UCHAR \
    CTL_CODE( ROMEO_TYPE, 0x901, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_ROMEO_OPN3_SETREG \
    CTL_CODE( ROMEO_TYPE, 0x902, METHOD_BUFFERED, FILE_WRITE_ACCESS )

#define IOCTL_ROMEO_OPN3_RESET \
    CTL_CODE( ROMEO_TYPE, 0x903, METHOD_BUFFERED, FILE_WRITE_ACCESS )

typedef struct  _ROMEO_WRITE_INPUT 
{
    ULONG   PortNumber;     // Port # to write to
    union   {               // Data to be output to port
        ULONG   LongData;
        USHORT  ShortData;
        UCHAR   CharData;
    };
}   ROMEO_WRITE_INPUT, *PROMEO_WRITE_INPUT;

typedef struct ROMEO_WRITEREG_t
{
	UCHAR	code;
	UCHAR	data;
	USHORT	port;
} ROMEO_WRITEREG;

#define PICCOLO_WRITE		0x00
#define PICCOLO_WAITTIMER	0x01

#define IOCTL_ROMEO_OPN3_WRITE \
	CTL_CODE( ROMEO_TYPE, 0x904, METHOD_BUFFERED, FILE_WRITE_ACCESS )

