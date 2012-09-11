//	$Id: sine.h,v 1.1 1999/10/10 01:41:59 cisc Exp $

#ifndef incl_sine_h
#define incl_sine_h

#include "device.h"
#include "if/ifcommon.h"

// ---------------------------------------------------------------------------

class Sine : public Device, public ISoundSource
{
public:
	enum IDFunc
	{
		setvolume = 0, setpitch
	};

public:
	Sine();
	~Sine();
	
	bool Init();
	void Cleanup();	
	
	// ISoundSource method
	bool IFCALL Connect(ISoundControl* sc);
	bool IFCALL SetRate(uint rate);
	void IFCALL Mix(int32*, int);
		
	// IDevice Method
	const Descriptor* IFCALL GetDesc() const { return &descriptor; }
	
	// I/O port functions
	void IOCALL SetVolume(uint, uint data);
	void IOCALL SetPitch(uint, uint data);

private:
	ISoundControl* sc;

	int volume;
	int rate;
	int pitch;
	int pos;
	int step;

	static const int table[];
	
	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};

#endif // incl_sine_h
