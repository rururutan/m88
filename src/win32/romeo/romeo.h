#ifndef _ROMEO_ROMEO_H
#define _ROMEO_ROMEO_H

#define	ROMEO_VENDORID		0x6809
#define	ROMEO_DEVICEID		0x2151
#define	ROMEO_DEVICEID2		0x8121		/* for Developer version */

enum {
	PCIERR_SUCCESS			= 0x00,
	PCIERR_INVALIDCLASS		= 0x83,
	PCIERR_DEVNOTFOUND		= 0x86
};

enum {
	ROMEO_DEVICE_VENDOR		= 0x00,
	ROMEO_STATUS_COMMAND	= 0x04,
	ROMEO_CLASS_REVISON		= 0x08,
	ROMEO_HEADTYPE			= 0x0c,
	ROMEO_BASEADDRESS0		= 0x10,
	ROMEO_BASEADDRESS1		= 0x14,
	ROMEO_SUB_DEVICE_VENDOR	= 0x2c,
	ROMEO_PCIINTERRUPT		= 0x3c
};

enum {
	ROMEO_YM2151BASE		= 0x0000,
	ROMEO_YMF288BASE		= 0x0100,
};


#define	PCIDEBUG_DLL	"pcidebug.dll"

typedef ULONG (WINAPI *PCIFINDDEV)(ULONG ven, ULONG dev, ULONG index);
typedef ULONG (WINAPI *PCICFGREAD32)(ULONG pciaddr, ULONG regaddr);
typedef void (WINAPI *PCIMEMWR8)(ULONG addr, UCHAR param);
typedef void (WINAPI *PCIMEMWR16)(ULONG addr, USHORT param);
typedef void (WINAPI *PCIMEMWR32)(ULONG addr, ULONG param);
typedef UCHAR (WINAPI *PCIMEMRD8)(ULONG addr);
typedef USHORT (WINAPI *PCIMEMRD16)(ULONG addr);
typedef ULONG (WINAPI *PCIMEMRD32)(ULONG addr);

#define	FN_PCIFINDDEV	"_pciFindPciDevice"
#define	FN_PCICFGREAD32	"_pciConfigReadLong"
#define	FN_PCIMEMWR8	"_MemWriteChar"
#define	FN_PCIMEMWR16	"_MemWriteShort"
#define	FN_PCIMEMWR32	"_MemWriteLong"
#define	FN_PCIMEMRD8	"_MemReadChar"
#define	FN_PCIMEMRD16	"_MemReadShort"
#define	FN_PCIMEMRD32	"_MemReadLong"

#define pciBusDevFunc(Bus,Dev,Func)		( ((Bus&0xff)<<8)|((Dev&0x1f)<<3)|(Func&7) )

#endif /* _ROMEO_ROMEO_H */
