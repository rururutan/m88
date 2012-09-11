// ---------------------------------------------------------------------------
//	class InstanceThunk
//	Copyright (C) cisc 1997.
// ---------------------------------------------------------------------------
//	$Id: instthnk.h,v 1.3 1999/07/31 12:42:19 cisc Exp $

#if !defined(WIN32_INSTANCETHUNK_H)
#define WIN32_INSTANCETHUNK_H

// ---------------------------------------------------------------------------
//	InstanceThunk
//	WinProc などのコールバック関数を C++ クラスに適合し易くするための
//	あやしげな手段．
//	
//	static で _stdcall 属性を持つ関数を呼び出す際に，
//	引数の最初に初期化時に指定したポインタ変数を付加することができる．
//	
//	MFC は hashed map で切り抜けているようだけど．OWL が同じような手法を
//	使っているみたい．
//	
//	使い方を間違えると即暴走するので気をつけて．
//	
class InstanceThunk 
{
public:
	InstanceThunk() {}
	~InstanceThunk() {}

	void SetDestination(void* func, void* arg0);
	operator void*() { return EntryThunk; }

private:
	BYTE EntryThunk[16];
};

#endif // !defined(WIN32_INSTANCETHUNK_H)
