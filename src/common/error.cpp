// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	$Id: error.cpp,v 1.6 2002/04/07 05:40:08 cisc Exp $

#include "headers.h"
#include "error.h"

Error::Errno Error::err = Error::unknown;

const char* Error::ErrorText[Error::nerrors] =
{
	"原因不明のエラーが発生しました.",
	"PC88 の ROM ファイルが見つかりません.\nファイル名を確認してください.",
	"メモリの割り当てに失敗しました.",
	"画面の初期化に失敗しました.",
	"スレッドを作成できません.",
	"テキストフォントが見つかりません.",
	"実行ファイルが書き換えられた恐れがあります.\n故意でなければウィルス感染が疑われます.",
};

const char* Error::GetErrorText()
{
	return ErrorText[err];
}

void Error::SetError(Errno e)
{
	err = e;
}
