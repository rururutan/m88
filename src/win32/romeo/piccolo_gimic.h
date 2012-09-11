//	$Id: piccolo.h,v 1.2 2002/05/31 09:45:22 cisc Exp $

#pragma once

#include "piccolo.h"

class Piccolo_Gimic : public Piccolo
{
public:
	Piccolo_Gimic();
	virtual ~Piccolo_Gimic();
	int Init();
	int GetChip( PICCOLO_CHIPTYPE type, PiccoloChip** pc );
	void SetReg( uint addr, uint data );
private:
};

