#pragma once

struct IDMAAccess
{
	virtual uint IFCALL RequestRead (uint bank, uint8* data, uint nbytes) = 0;
	virtual uint IFCALL RequestWrite(uint bank, uint8* data, uint nbytes) = 0;
};

