// ---------------------------------------------------------------------------
//	class InstanceThunk
//	Copyright (C) cisc 1997.
// ---------------------------------------------------------------------------
//	$Id: instthnk.cpp,v 1.3 1999/07/22 15:57:28 cisc Exp $

#include "headers.h"
#include "instthnk.h"

// ---------------------------------------------------------------------------
//	呼び出し先設定
//
void InstanceThunk::SetDestination(void* func, void* arg0)
{
	static const BYTE EntryCode[16] =
	{
		0xff, 0x34, 0x24,								// 00:push [esp]
		0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00,	// 03:mov  [esp+4], "this"
		0xe9, 0x00, 0x00, 0x00, 0x00,					// 0B:jmp "calladdress"
	};
	memcpy(EntryThunk, EntryCode, 16);
	*((void**) &EntryThunk[ 7]) = arg0;
	*((int*) &EntryThunk[12]) = (BYTE*) func - (EntryThunk + 16);

//	何かあったらコメントアウトしてみるのもいいかも
//	DWORD old;
//	VirtualProtect(EntryThunk, 16, PAGE_EXECUTE_READWRITE, &old);
}
