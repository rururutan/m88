//	$Id: piccolo.h,v 1.2 2002/05/31 09:45:22 cisc Exp $

#pragma once

#include "piccolo.h"

class Piccolo_Romeo : public Piccolo
{
public:
	Piccolo_Romeo();
	virtual ~Piccolo_Romeo();
	int Init();
	int GetChip(PICCOLO_CHIPTYPE type, PiccoloChip** pc);
	void SetReg( uint addr, uint data );

	void Reset();
	void Mute();
	bool IsBusy();
private:
	enum
	{
		ADDR0 = 0x00,
		DATA0 = 0x04,
		ADDR1 = 0x08,
		DATA1 = 0x0c,
		CTRL  = 0x1c,
	};

private:
};

