//	$Id: sine.cpp,v 1.1 1999/10/10 01:41:59 cisc Exp $

#include "headers.h"
#include "sine.h"

Sine::Sine()
: Device(0), volume(0), rate(0), pitch(0), pos(0), sc(0)
{
}

Sine::~Sine()
{
}

bool Sine::Init()
{
	volume = 64;
	rate = 0;
	pitch = 64;
	pos = 0;
	return true;
}

bool IFCALL Sine::Connect(ISoundControl* _sc)
{
	if (sc) sc->Disconnect(this);
	sc = _sc;
	if (sc) sc->Connect(this);
	return true;
}

bool IFCALL Sine::SetRate(uint r)
{
	rate = r;
	step = 4000 * 128 * pitch / rate;
	return true;
}

void IOCALL Sine::SetPitch(uint, uint data)
{
	sc->Update(this);
	pitch = data;
	step = 4000 * 128 * pitch / rate;
}

void IOCALL Sine::SetVolume(uint, uint data)
{
	sc->Update(this);
	volume = data;
}

void IFCALL Sine::Mix(int32* dest, int length)
{
	for (; length > 0; length --)
	{
		int a = (table[(pos >> 8) & 127] * volume) >> 8;
		pos += step;
		dest[0] += a;		// バッファには既に他の音源の合成結果が
		dest[1] += a;			// 含まれているので「加算」すること！
		dest += 2;
	}
}

const int Sine::table[] = 
{
		 0,    392,    784,   1173,   1560,   1943,   2322,   2695,
	  3061,   3420,   3771,   4112,   4444,   4765,   5075,   5372,
	  5656,   5927,   6184,   6425,   6651,   6861,   7055,   7231,
	  7391,   7532,   7655,   7760,   7846,   7913,   7961,   7990,
	  7999,   7990,   7961,   7913,   7846,   7760,   7655,   7532,
	  7391,   7231,   7055,   6861,   6651,   6425,   6184,   5927,
	  5656,   5372,   5075,   4765,   4444,   4112,   3771,   3420,
	  3061,   2695,   2322,   1943,   1560,   1173,    784,    392,
		 0,   -392,   -784,  -1173,  -1560,  -1943,  -2322,  -2695,
	 -3061,  -3420,  -3771,  -4112,  -4444,  -4765,  -5075,  -5372,
	 -5656,  -5927,  -6184,  -6425,  -6651,  -6861,  -7055,  -7231,
	 -7391,  -7532,  -7655,  -7760,  -7846,  -7913,  -7961,  -7990,
	 -7999,  -7990,  -7961,  -7913,  -7846,  -7760,  -7655,  -7532,
	 -7391,  -7231,  -7055,  -6861,  -6651,  -6425,  -6184,  -5927,
	 -5656,  -5372,  -5075,  -4765,  -4444,  -4112,  -3771,  -3420,
	 -3061,  -2695,  -2322,  -1943,  -1560,  -1173,   -784,   -392,
};

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor Sine::descriptor = { indef, outdef };

const Device::OutFuncPtr Sine::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, SetVolume),
	STATIC_CAST(Device::OutFuncPtr, SetPitch),
};

const Device::InFuncPtr Sine::indef[] = 
{
	0,
};


