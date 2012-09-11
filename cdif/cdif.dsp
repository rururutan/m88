# Microsoft Developer Studio Project File - Name="cdif" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cdif - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "cdif.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "cdif.mak" CFG="cdif - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "cdif - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "cdif - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cdif - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CDIF_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "..\src" /I "..\src\win32" /I "..\src\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CDIF_EXPORTS" /FAc /Yu"headers.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /machine:I386 /out:"../Release/cdif.m88"

!ELSEIF  "$(CFG)" == "cdif - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CDIF_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "..\src" /I "..\src\win32" /I "..\src\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CDIF_EXPORTS" /Yu"headers.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../Debug/cdif.m88" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "cdif - Win32 Release"
# Name "cdif - Win32 Debug"
# Begin Group "pch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pch.cpp
# ADD CPP /Yc"headers.h"
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\common\device.h
# End Source File
# Begin Source File

SOURCE=..\src\win32\diag.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Win32\diag.h
# End Source File
# Begin Source File

SOURCE=..\src\if\ifcommon.h
# End Source File
# Begin Source File

SOURCE=..\src\if\ifpc88.h
# End Source File
# Begin Source File

SOURCE=..\src\common\misc.h
# End Source File
# End Group
# Begin Group "scsi"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aspi.cpp
# End Source File
# Begin Source File

SOURCE=.\src\aspi.h
# End Source File
# Begin Source File

SOURCE=.\src\aspidef.h
# End Source File
# Begin Source File

SOURCE=.\src\cdrom.cpp
# End Source File
# Begin Source File

SOURCE=.\src\cdrom.h
# End Source File
# Begin Source File

SOURCE=.\src\cdromdef.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\src\cdctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\cdctrl.h
# End Source File
# Begin Source File

SOURCE=.\src\cdif.bmp
# End Source File
# Begin Source File

SOURCE=.\src\cdif.cpp
# End Source File
# Begin Source File

SOURCE=.\src\cdif.h
# End Source File
# Begin Source File

SOURCE=.\src\cdif.ico
# End Source File
# Begin Source File

SOURCE=.\src\cdif.rc
# End Source File
# Begin Source File

SOURCE=.\src\config.cpp
# End Source File
# Begin Source File

SOURCE=.\src\config.h
# End Source File
# Begin Source File

SOURCE=.\src\guid.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\headers.h
# End Source File
# Begin Source File

SOURCE=..\src\win32\instthnk.cpp
# End Source File
# Begin Source File

SOURCE=..\src\win32\instthnk.h
# End Source File
# Begin Source File

SOURCE=.\src\moduleif.cpp
# End Source File
# Begin Source File

SOURCE=.\src\resource.h
# End Source File
# End Target
# End Project
