// ---------------------------------------------------------------------------
//	IIR LPF
//	Copyright (C) cisc 2001.
// ---------------------------------------------------------------------------
//	$Id: lpf.cpp,v 1.1 2002/04/07 05:40:08 cisc Exp $

#include "headers.h"
#include "lpf.h"
#include "misc.h"

// ---------------------------------------------------------------------------
//	バタワース特性 IIR IIR_LPF
//
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#define FX(f)	int((f) * F)

void IIR_LPF::MakeFilter(uint fc, uint samplingrate, uint _order)
{
	order = Limit(_order, maxorder, 1);

	double wa = tan(M_PI * fc / samplingrate);
	double wa2 = wa*wa;

	if (fc > samplingrate / 2)
		fc = samplingrate / 2 - 1000;

	uint j;
	int n = 1;
    for (j=0; j<order; j++)
    {
		double zt = cos(n * M_PI / 4 / order);
		double ia0j = 1. / (1. + 2. * wa * zt + wa2);
		
		fn[j][0] = FX(-2. * (wa2 - 1.) * ia0j);
		fn[j][1] = FX(-(1. - 2. * wa * zt + wa2) * ia0j);
		fn[j][2] = FX(wa2 * ia0j);
		fn[j][3] = FX(2. * wa2 * ia0j);
		n += 2;
    }

	for (int ch=0; ch<nchs; ch++)
	{
		for (j=0; j<order; j++)
		{
			b[ch][j][0] = b[ch][j][1] = 0;
		}
	}
}
