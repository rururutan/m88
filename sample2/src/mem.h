//	$Id: mem.h,v 1.1 1999/10/10 01:43:28 cisc Exp $

#ifndef incl_mem_h
#define incl_mem_h

#include "device.h"
#include "if/ifcommon.h"

// ---------------------------------------------------------------------------

class GVRAMReverse : public Device
{
public:
	enum
	{
		out32 = 0, out35, out5x
	};

public:
	GVRAMReverse();
	~GVRAMReverse();
	
	bool Init(IMemoryManager* mm);
	void Cleanup();	
			
	// IDevice Method
	const Descriptor* IFCALL GetDesc() const { return &descriptor; }
	
	// I/O port functions
	void IOCALL Out32(uint, uint data);
	void IOCALL Out35(uint, uint data);
	void IOCALL Out5x(uint, uint data);

	static uint MEMCALL MRead(void*, uint);
	static void MEMCALL MWrite(void*, uint, uint);

private:
	void Update();

	IMemoryManager* mm;
	int mid;

	uint p32, p35, p5x;
	bool gvram;

	static const Descriptor descriptor;
//	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};

#endif // incl_mem_h
