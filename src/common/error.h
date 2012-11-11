// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	$Id: error.h,v 1.2 1999/12/07 00:14:14 cisc Exp $

#pragma once

class Error
{
public:
	enum Errno
	{
		unknown = 0,
		NoROM,
		OutOfMemory,
		ScreenInitFailed,
		ThreadInitFailed,
		LoadFontFailed,
		InsaneModule,
		nerrors
	};

public:
	static void SetError(Errno e);
	static const char* GetErrorText();

private:
	Error();

	static Errno err;
	static const char* ErrorText[nerrors];
};

