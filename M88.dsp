# Microsoft Developer Studio Project File - Name="M88" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=M88 - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "M88.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "M88.mak" CFG="M88 - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "M88 - Win32 Release" ("Win32 (x86) Application" 用)
!MESSAGE "M88 - Win32 Debug" ("Win32 (x86) Application" 用)
!MESSAGE "M88 - Win32 Tuning" ("Win32 (x86) Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "M88 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /Ox /Ot /Oa /Og /Oi /I ".\src" /I ".\src\Win32" /I ".\src\common" /I ".\src\devices" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FAc /Fr /Yu"headers.h" /FD /c
# SUBTRACT CPP /Ow /Os
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /subsystem:windows /map /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=writetag release\m88.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "M88 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\src" /I ".\src\Win32" /I ".\src\PC88" /I ".\src\common" /I ".\src\devices" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"headers.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib ddraw.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "M88 - Win32 Tuning"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "M88___Win32_Tuning"
# PROP BASE Intermediate_Dir "M88___Win32_Tuning"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Tuning"
# PROP Intermediate_Dir "Tuning"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MT /W3 /GX /Ox /Ot /Og /Oi /I ".\src" /I ".\src\Win32" /I ".\src\common" /I ".\src\devices" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FA /YX".\win32\headers.h" /FD /c
# SUBTRACT BASE CPP /Ow
# ADD CPP /nologo /G6 /MDd /W3 /GX /Zi /Ox /Ot /Og /Oi /I ".\src" /I ".\src\Win32" /I ".\src\common" /I ".\src\devices" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_DEBUG" /Yu"headers.h" /FD /c
# SUBTRACT CPP /Ow
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /subsystem:windows /map /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "M88 - Win32 Release"
# Name "M88 - Win32 Debug"
# Name "M88 - Win32 Tuning"
# Begin Group "pch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Win32\pch.cpp

!IF  "$(CFG)" == "M88 - Win32 Release"

# ADD CPP /Yc""

!ELSEIF  "$(CFG)" == "M88 - Win32 Debug"

# ADD CPP /Yc

!ELSEIF  "$(CFG)" == "M88 - Win32 Tuning"

# ADD CPP /Yc

!ENDIF 

# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\common\device.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\device.h
# End Source File
# Begin Source File

SOURCE=.\src\common\device_i.h
# End Source File
# Begin Source File

SOURCE=.\src\common\draw.h
# End Source File
# Begin Source File

SOURCE=.\src\common\error.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\error.h
# End Source File
# Begin Source File

SOURCE=.\src\common\lpf.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\lpf.h
# End Source File
# Begin Source File

SOURCE=.\src\common\lz77d.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\lz77d.h
# End Source File
# Begin Source File

SOURCE=.\src\common\memmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\memmgr.h
# End Source File
# Begin Source File

SOURCE=.\src\common\misc.h
# End Source File
# Begin Source File

SOURCE=.\src\common\schedule.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\schedule.h
# End Source File
# Begin Source File

SOURCE=.\src\common\sndbuf2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\sndbuf2.h
# End Source File
# Begin Source File

SOURCE=.\src\common\soundbuf.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\soundbuf.h
# End Source File
# Begin Source File

SOURCE=.\src\common\soundsrc.h
# End Source File
# Begin Source File

SOURCE=.\src\common\srcbuf.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\srcbuf.h
# End Source File
# End Group
# Begin Group "devices"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\devices\fmgen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\devices\fmgen.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\fmgeninl.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\fmtimer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\devices\fmtimer.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\opm.cpp

!IF  "$(CFG)" == "M88 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M88 - Win32 Debug"

!ELSEIF  "$(CFG)" == "M88 - Win32 Tuning"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\devices\opm.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\opna.cpp
# End Source File
# Begin Source File

SOURCE=.\src\devices\opna.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\psg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\devices\psg.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80_x86.cpp
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80_x86.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80c.cpp

!IF  "$(CFG)" == "M88 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M88 - Win32 Debug"

!ELSEIF  "$(CFG)" == "M88 - Win32 Tuning"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80c.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80debug.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80debug.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80diag.cpp
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80diag.h
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80Test.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\devices\Z80Test.h
# End Source File
# End Group
# Begin Group "PC88"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pc88\base.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\base.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\beep.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\beep.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\calender.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\calender.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\config.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\crtc.cpp

!IF  "$(CFG)" == "M88 - Win32 Release"

# ADD CPP /Ox /Ot /Oa /Og /Oi /Op- /Oy /Ob1
# SUBTRACT CPP /Os

!ELSEIF  "$(CFG)" == "M88 - Win32 Debug"

!ELSEIF  "$(CFG)" == "M88 - Win32 Tuning"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\pc88\crtc.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\diskmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\diskmgr.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\fdc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\fdc.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\fdu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\fdu.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\floppy.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\floppy.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\intc.cpp

!IF  "$(CFG)" == "M88 - Win32 Release"

# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "M88 - Win32 Debug"

!ELSEIF  "$(CFG)" == "M88 - Win32 Tuning"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\pc88\intc.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\ioview.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\ioview.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\joypad.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\joypad.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\kanjirom.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\kanjirom.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\memory.cpp

!IF  "$(CFG)" == "M88 - Win32 Release"

!ELSEIF  "$(CFG)" == "M88 - Win32 Debug"

!ELSEIF  "$(CFG)" == "M88 - Win32 Tuning"

# ADD BASE CPP /FA
# ADD CPP /FA

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\pc88\memory.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\memview.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\memview.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\mouse.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\mouse.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\opnif.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\opnif.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\pc88.cpp
# End Source File
# Begin Source File

SOURCE=.\src\PC88\pc88.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\pcinfo.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\pd8257.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\pd8257.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\pio.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\pio.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\screen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\screen.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\sio.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\sio.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\sound.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\sound.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\subsys.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\subsys.h
# End Source File
# Begin Source File

SOURCE=.\src\pc88\tapemgr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\pc88\tapemgr.h
# End Source File
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Group "romeo"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\win32\romeo\juliet.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\win32\romeo\juliet.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\romeo\piccolo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\romeo\piccolo.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\romeo\romeo.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\Win32\88config.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\88config.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\about.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\about.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\basmon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\basmon.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\cfgpage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\cfgpage.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\codemon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\codemon.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\CritSect.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\dderr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\dderr.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\diag.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\diag.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\DrawDDS.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\DrawDDS.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\DrawDDW.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\DrawDDW.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\DrawGDI.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\DrawGDI.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\extdev.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\extdev.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\File.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\File.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\filetest.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\filetest.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\guid.cpp

!IF  "$(CFG)" == "M88 - Win32 Release"

# SUBTRACT CPP /Fr /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "M88 - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "M88 - Win32 Tuning"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\headers.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\instthnk.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\instthnk.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\iomon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\iomon.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\keybconn.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\win32\keybconn.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\loadmon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\loadmon.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\M88.rc
# End Source File
# Begin Source File

SOURCE=.\src\Win32\main.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\memmon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\memmon.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\messages.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\module.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\module.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\mvmon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\mvmon.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\newdisk.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\newdisk.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\regmon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\regmon.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\resource.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\sequence.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\sequence.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\sounddrv.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\soundds.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\soundds.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\soundds2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\soundds2.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\soundmon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\soundmon.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\soundwo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\soundwo.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\status.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\status.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\timekeep.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\timekeep.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\types.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\Ui.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\ui.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\version.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\wincfg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\wincfg.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\wincore.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\wincore.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\windraw.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\windraw.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\winexapi.cpp

!IF  "$(CFG)" == "M88 - Win32 Release"

# ADD CPP /FA

!ELSEIF  "$(CFG)" == "M88 - Win32 Debug"

!ELSEIF  "$(CFG)" == "M88 - Win32 Tuning"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\winexapi.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\WinJoy.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\WinJoy.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\WinKeyIF.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\WinKeyIF.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\winmon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\winmon.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\winmouse.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\winmouse.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\winsound.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\winsound.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\winvars.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32\winvars.h
# End Source File
# End Group
# Begin Group "interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\if\ifcommon.h
# End Source File
# Begin Source File

SOURCE=.\src\if\ifguid.h
# End Source File
# Begin Source File

SOURCE=.\src\if\ifpc88.h
# End Source File
# Begin Source File

SOURCE=.\src\if\ifui.h
# End Source File
# End Group
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\zlib\adler32.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\compress.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\crc32.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\deflate.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\gzio.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\infblock.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\infblock.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\infcodes.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\infcodes.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inffast.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inflate.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inftrees.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\infutil.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\infutil.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\trees.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\uncompr.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zutil.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zutil.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\Win32\M88.ico
# End Source File
# Begin Source File

SOURCE=.\m88dev.html
# End Source File
# Begin Source File

SOURCE=.\memo.txt
# End Source File
# End Target
# End Project
