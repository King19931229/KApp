# Microsoft Developer Studio Project File - Name="ex_csvcode" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ex_csvcode - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ex_csvcode.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ex_csvcode.mak" CFG="ex_csvcode - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ex_csvcode - Win32 Release x86" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - Win32 Debug x86" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - Win32 ASCII Debug x86" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - Win32 ASCII Release x86" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - x64 Debug AMD64" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - x64 Release AMD64" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - x64 Debug IA64" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - x64 Release IA64" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - Win32 Debug AMD64" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - Win32 Release AMD64" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - Win32 Debug IA64" (based on "Win32 (x86) Console Application")
!MESSAGE "ex_csvcode - Win32 Release IA64" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ex_csvcode - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release/ex_csvcode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 libdb47.lib  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libcmt" /libpath:"Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug/ex_csvcode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libdb47d.lib  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /nodefaultlib:"libcmtd" /fixed:no /libpath:"Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - Win32 ASCII Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_ASCII"
# PROP BASE Intermediate_Dir "Debug_ASCII/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_ASCII"
# PROP Intermediate_Dir "Debug_ASCII/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47d.lib  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /nodefaultlib:"libcmtd" /fixed:no
# ADD LINK32 libdb47d.lib  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /nodefaultlib:"libcmtd" /fixed:no /libpath:"Debug_ASCII"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - Win32 ASCII Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_ASCII"
# PROP BASE Intermediate_Dir "Release_ASCII/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_ASCII"
# PROP Intermediate_Dir "Release_ASCII/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47.lib  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libcmt"
# ADD LINK32 libdb47.lib  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libcmt" /libpath:"Release_ASCII"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - x64 Debug AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_AMD64"
# PROP BASE Intermediate_Dir "Debug_AMD64/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_AMD64"
# PROP Intermediate_Dir "Debug_AMD64/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47d.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:AMD64 /nodefaultlib:"libcmtd" /fixed:no
# ADD LINK32 libdb47d.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:AMD64 /nodefaultlib:"libcmtd" /fixed:no /libpath:"Debug_AMD64"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - x64 Release AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_AMD64"
# PROP BASE Intermediate_Dir "Release_AMD64/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_AMD64"
# PROP Intermediate_Dir "Release_AMD64/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /EHsc /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD CPP /nologo /MD /W3 /EHsc /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:AMD64 /nodefaultlib:"libcmt"
# ADD LINK32 libdb47.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:AMD64 /nodefaultlib:"libcmt" /libpath:"Release_AMD64"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - x64 Debug IA64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_IA64"
# PROP BASE Intermediate_Dir "Debug_IA64/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_IA64"
# PROP Intermediate_Dir "Debug_IA64/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47d.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:IA64 /nodefaultlib:"libcmtd" /fixed:no
# ADD LINK32 libdb47d.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:IA64 /nodefaultlib:"libcmtd" /fixed:no /libpath:"Debug_IA64"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - x64 Release IA64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_IA64"
# PROP BASE Intermediate_Dir "Release_IA64/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_IA64"
# PROP Intermediate_Dir "Release_IA64/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /EHsc /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD CPP /nologo /MD /W3 /EHsc /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:IA64 /nodefaultlib:"libcmt"
# ADD LINK32 libdb47.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:IA64 /nodefaultlib:"libcmt" /libpath:"Release_IA64"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - Win32 Debug AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_AMD64"
# PROP BASE Intermediate_Dir "Debug_AMD64/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_AMD64"
# PROP Intermediate_Dir "Debug_AMD64/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47d.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:AMD64 /nodefaultlib:"libcmtd" /fixed:no
# ADD LINK32 libdb47d.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:AMD64 /nodefaultlib:"libcmtd" /fixed:no /libpath:"Debug_AMD64"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - Win32 Release AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_AMD64"
# PROP BASE Intermediate_Dir "Release_AMD64/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_AMD64"
# PROP Intermediate_Dir "Release_AMD64/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /EHsc /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD CPP /nologo /MD /W3 /EHsc /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:AMD64 /nodefaultlib:"libcmt"
# ADD LINK32 libdb47.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:AMD64 /nodefaultlib:"libcmt" /libpath:"Release_AMD64"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - Win32 Debug IA64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_IA64"
# PROP BASE Intermediate_Dir "Debug_IA64/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_IA64"
# PROP Intermediate_Dir "Debug_IA64/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47d.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:IA64 /nodefaultlib:"libcmtd" /fixed:no
# ADD LINK32 libdb47d.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:IA64 /nodefaultlib:"libcmtd" /fixed:no /libpath:"Debug_IA64"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ex_csvcode - Win32 Release IA64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_IA64"
# PROP BASE Intermediate_Dir "Release_IA64/ex_csvcode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_IA64"
# PROP Intermediate_Dir "Release_IA64/ex_csvcode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /EHsc /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD CPP /nologo /MD /W3 /EHsc /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /Wp64 /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb47.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:IA64 /nodefaultlib:"libcmt"
# ADD LINK32 libdb47.lib  bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:IA64 /nodefaultlib:"libcmt" /libpath:"Release_IA64"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generate csv_local.c
PostBuild_Cmds=$(OUTDIR)\ex_csvcode.exe -c ../examples_c/csv/csv_local.c -h ../examples_c/csv/csv_local.h -f ../examples_c/csv/sample.desc
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ex_csvcode - Win32 Release x86"
# Name "ex_csvcode - Win32 Debug x86"
# Name "ex_csvcode - Win32 ASCII Debug x86"
# Name "ex_csvcode - Win32 ASCII Release x86"
# Name "ex_csvcode - x64 Debug AMD64"
# Name "ex_csvcode - x64 Release AMD64"
# Name "ex_csvcode - x64 Debug IA64"
# Name "ex_csvcode - x64 Release IA64"
# Name "ex_csvcode - Win32 Debug AMD64"
# Name "ex_csvcode - Win32 Release AMD64"
# Name "ex_csvcode - Win32 Debug IA64"
# Name "ex_csvcode - Win32 Release IA64"
# Begin Source File

SOURCE=..\examples_c\csv\code.c
# End Source File
# Begin Source File

SOURCE=..\clib\getopt.c
# End Source File
# End Target
# End Project
