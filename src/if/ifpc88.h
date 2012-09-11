#ifndef incl_interface_pc88_h
#define incl_interface_pc88_h

struct IDMAAccess
{
	virtual uint IFCALL RequestRead (uint bank, uint8* data, uint nbytes) = 0;
	virtual uint IFCALL RequestWrite(uint bank, uint8* data, uint nbytes) = 0;
};

#endif // incl_interface_pc88_h
