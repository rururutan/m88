// $Id: dderr.h,v 1.1 2000/02/09 10:47:38 cisc Exp $

#pragma once

#ifdef LOGGING

const char* GetDDERR(HRESULT hr);

inline void LOGDDERR(const char* text, HRESULT hr)
{
	if (hr != DD_OK)
	{
		const char* err = GetDDERR(hr);
		if (!err)
			LOG2("%s -> 0x%.8x\n", text, hr);
		else
			LOG2("%s -> %s\n", text, err);
	}
	return;
}

#else
#define LOGDDERR(tx, hr)	((void)(0))
#endif

