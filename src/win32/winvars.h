//	$Id: winvars.h,v 1.1 2000/11/02 12:43:52 cisc Exp $

#pragma once

class WinVars
{
public:
	enum Type
	{
		MajorVer = 0,
		MinorVer,
		OFNSIZE,
		MIISIZE,
		nparam
	};

	WinVars() { Init(); }

	static int Var(Type t) { return var[t]; }

private:
	static void Init();
	
	static int var[nparam];
};

#define WINVAR(a) WinVars::Var(WinVars::a)

