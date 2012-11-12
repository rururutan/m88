// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1997, 2000.
// ---------------------------------------------------------------------------
//	$Id: tapemgr.cpp,v 1.3 2000/08/06 09:58:51 cisc Exp $

#include "headers.h"
#include "tapemgr.h"
#include "file.h"
#include "status.h"
#include "misc.h"


#define LOGNAME	"tape"
#include "diag.h"

#define T88ID	"PC-8801 Tape Image(T88)"

// ---------------------------------------------------------------------------
//	構築
//
TapeManager::TapeManager()
: Device(DEV_ID('T','A','P','E')), tags(0), scheduler(0), event(0)
{
	datasize = 0;
	datatype = 0;
	pinput = 0;
	pos = 0;
	bus = 0;
	offset = 0;
	mode = T_BLANK;
	motor = false;
}

// ---------------------------------------------------------------------------
//	破棄
//
TapeManager::~TapeManager()
{
	Close();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool TapeManager::Init(Scheduler* s, IOBus* b, int pi)
{
	scheduler = s;
	bus = b;
	pinput = pi;

	motor = false;
	timercount = 0;
	timerremain = 0;
	tick = 0;
	return true;
}
	
// ---------------------------------------------------------------------------
//	T88 を開く
//
bool TapeManager::Open(const char* file)
{
	Close();

	FileIO fio;
	if (!fio.Open(file, FileIO::readonly))
		return false;

	// ヘッダ確認
	char buf[24];
	fio.Read(buf, 24);
	if (memcmp(buf, T88ID, 24))
		return false;

	// タグのリスト構造を展開
	Tag* prv = 0;
	do
	{
		TagHdr hdr;
		fio.Read(&hdr, 4);
		
		Tag* tag = (Tag*) new uchar[sizeof(Tag)-1+hdr.length];
		if (!tag)
		{
			Close();
			return false;
		}
		
		tag->prev = prv;
		tag->next = 0;
		(prv ? prv->next : tags) = tag;
		tag->id = hdr.id;
		tag->length = hdr.length;
		fio.Read(tag->data, tag->length);
		prv = tag;
	} while (prv->id);

	if (!Rewind())
		return false;
	return true;
}

// ---------------------------------------------------------------------------
//	とじる
//
bool TapeManager::Close()
{
	if (scheduler)
		SetTimer(0);
	while (tags)
	{
		Tag* n = tags->next;
		delete []tags;
		tags = n;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	まきもどす
//
bool TapeManager::Rewind(bool timer)
{
	pos = tags;
	scheduler->DelEvent(event), event = 0;
	if (pos)
	{
		tick = 0;

		// バージョン確認
		// 最初のタグはバージョンタグになるはず？
		if (   pos->id != T_VERSION
			|| pos->length < 2 
			|| *(uint16*)pos->data != T88VER)
			return false;
		
		pos = pos->next;
		Proceed(timer);
	}
	return true;
}

// ---------------------------------------------------------------------------
//	モータ
//
bool TapeManager::Motor(bool s)
{
	if (motor == s)
		return true;
	if (s)
	{
		statusdisplay.Show(10, 2000, "Motor on: %d %d", timerremain, timercount);
		time = scheduler->GetTime();
		if (timerremain)
			event = scheduler->AddEvent(timercount*125/6, this, STATIC_CAST(TimeFunc, &TapeManager::Timer));
		motor = true;
	}
	else
	{
		if (timercount)
		{
			int td = (scheduler->GetTime() - time) * 6 / 125;
			timerremain = Max(10, timerremain - td);
			scheduler->DelEvent(event), event = 0;
			statusdisplay.Show(10, 2000, "Motor off: %d %d", timerremain, timercount);
		}
		motor = false;
	}
	return true;
}

// ---------------------------------------------------------------------------

uint TapeManager::GetPos()
{
	if (motor)
	{
		if (timercount)
			return tick + (scheduler->GetTime() - time) * 6 / 125;
		else
			return tick;
	}
	else
	{
		return tick + timercount - timerremain;
	}
}

// ---------------------------------------------------------------------------
//	タグを処理
//
void TapeManager::Proceed(bool timer)
{
	while (pos)
	{
		LOG1("TAG %d\n", pos->id);
		switch (pos->id)
		{
		case T_END:
			mode = T_BLANK;
			pos = 0;
			statusdisplay.Show(50, 0, "end of tape", tick);
			timercount = 0;
			return;

		case T_BLANK:
		case T_SPACE:
		case T_MARK:
		{
			BlankTag* t = (BlankTag*) pos->data;
			mode = (Mode) pos->id;

			if (t->pos + t->tick - tick <= 0)
				break;

			if (timer)
				SetTimer(t->pos + t->tick - tick);
			else
				timercount = t->pos + t->tick - tick;

			pos = pos->next;
			return;
		}

		case T_DATA:
		{
			DataTag* t = (DataTag*) pos->data;
			mode = T_DATA;

			data = t->data;
			datasize = t->length;
			datatype = t->type;
			offset = 0;

			if (!datasize)
				break;

			pos = pos->next;
			if (timer)
				SetTimer(datatype & 0x100 ? 44 : 88);
			else
				timercount = t->tick;
			return;
		}
		}
		pos = pos->next;
	}
}

// ---------------------------------------------------------------------------
//
//
void IOCALL TapeManager::Timer(uint)
{
	tick += timercount;
	statusdisplay.Show(50, 0, "tape: %d", tick);
	
	if (mode == T_DATA)
	{
		Send(*data++);
		offset++;
		if (--datasize > 0)
		{
			SetTimer(datatype & 0x100 ? 44 : 88);
			return;
		}
		LOG0("\n");
	}
	Proceed();
}

// ---------------------------------------------------------------------------
//	キャリア確認
//
bool TapeManager::Carrier()
{
	if (mode == T_MARK)
	{
		LOG0("*");
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	タイマー更新
//
void TapeManager::SetTimer(int count)
{
	if (count > 100)
		LOG1("Timer: %d\n", count);
	scheduler->DelEvent(event), event = 0;
	timercount = count;
	if (motor)
	{
		time = scheduler->GetTime();
		if (count)			// 100000/4800
			event = scheduler->AddEvent(count*125/6, this, STATIC_CAST(TimeFunc, &TapeManager::Timer));
	}
	else
		timerremain = count;
}

// ---------------------------------------------------------------------------
//	バイト転送
//
inline void TapeManager::Send(uint byte)
{
	LOG1("%.2x ", byte);
	bus->Out(pinput, byte);
}

// ---------------------------------------------------------------------------
//	即座にデータを要求する
//
void TapeManager::RequestData(uint, uint)
{
	if (mode == T_DATA)
	{
		scheduler->SetEvent(event, 1, this, STATIC_CAST(TimeFunc, &TapeManager::Timer));
	}
}

// ---------------------------------------------------------------------------
//	シークする
//
bool TapeManager::Seek(uint newpos, uint off)
{
	if (!Rewind(false))
		return false;

	while (pos && (tick + timercount) < newpos)
	{
		tick += timercount;
		Proceed(false);
	}
	if (!pos)
		return false;
	
	switch (pos->prev->id)
	{
		int l;

	case T_BLANK:
	case T_SPACE:
	case T_MARK:
		mode = (Mode) pos->prev->id;
		l = tick+timercount-newpos;
		tick = newpos;
		SetTimer(l);
		break;

	case T_DATA:
		mode = T_DATA;
		offset = off;
		newpos = tick + offset * (datatype ? 44 : 88);
		data += offset;
		datasize -= offset;
		SetTimer(datatype ? 44 : 88);
		break;

	default:
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------

void IOCALL TapeManager::Out30(uint, uint d)
{
	Motor(!!(d & 8));
}

uint IOCALL TapeManager::In40(uint)
{
	return IOBus::Active(Carrier() ? 4 : 0, 4);
}


// ---------------------------------------------------------------------------
//	状態保存
//
uint IFCALL TapeManager::GetStatusSize()
{
	return sizeof(Status);
}

bool IFCALL TapeManager::SaveStatus(uint8* s)
{
	Status* status = (Status*) s;
	status->rev = ssrev;
	status->motor = motor;
	status->pos = GetPos();
	status->offset = offset;
	statusdisplay.Show(0, 1000, "tapesave: %d", status->pos);
	return true;
}

bool IFCALL TapeManager::LoadStatus(const uint8* s)
{
	const Status* status = (const Status*) s;
	if (status->rev != ssrev)
		return false;
	motor = status->motor;
	Seek(status->pos, status->offset);
	statusdisplay.Show(0, 1000, "tapesave: %d", GetPos());
	return true;
}

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor TapeManager::descriptor =
{
	TapeManager::indef, TapeManager::outdef
};

const Device::OutFuncPtr TapeManager::outdef[] =
{
	STATIC_CAST(Device::OutFuncPtr, &RequestData),
	STATIC_CAST(Device::OutFuncPtr, &Out30),
};

const Device::InFuncPtr TapeManager::indef[] =
{
	STATIC_CAST(Device::InFuncPtr, &In40),
};
