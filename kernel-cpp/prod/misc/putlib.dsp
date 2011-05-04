# Microsoft Developer Studio Project File - Name="putlib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=putlib - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "putlib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "putlib.mak" CFG="putlib - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "putlib - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "putlib - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "putlib - Win32 Static" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "putlib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\putlib\Release"
# PROP BASE Intermediate_Dir ".\putlib\Release"
# PROP BASE Target_Dir ".\putlib"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "c:\bin"
# PROP Intermediate_Dir "q:\putlib\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\putlib"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /MD /W3 /GR /GX /Zi /Og /Os /Ob2 /Gf /D "_MBCS" /D "_DLL" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_USRDLL" /FAcs /FD /c
# SUBTRACT CPP /nologo /Ox /Ot /Oa /Ow /Oi /Gy /Fr /YX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /o"q:/putlib/Release/putlib.bsc"
# SUBTRACT BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 /subsystem:console /pdb:"q:\bin/putlib.pdb" /map:"q:\bin/putlib.map" /debug /machine:I386 /out:"q:\bin\\putlib.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "putlib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\putlib\Debug"
# PROP BASE Intermediate_Dir ".\putlib\Debug"
# PROP BASE Target_Dir ".\putlib"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "q:\putlib\Debug"
# PROP Intermediate_Dir "q:\putlib\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\putlib"
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /MDd /W3 /Gm /Gi /GR /GX /ZI /Od /Ob1 /D "_DEBUG" /D "_AFXDLL" /D "_MBCS" /D "_DLL" /D "WIN32" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /nologo /WX /Fr /YX
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 th32.lib /subsystem:console /pdb:"q:\bind\putlib.pdb" /debug /machine:I386 /out:"q:\bind\\putlib.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "putlib - Win32 Static"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "putlib\Static"
# PROP BASE Intermediate_Dir "putlib\Static"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir "putlib"
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "q:\putlib\Static"
# PROP Intermediate_Dir "q:\putlib\Static"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "putlib"
# ADD BASE CPP /MD /W3 /GR /GX /Zi /Og /Os /Ob2 /Gf /D "_MBCS" /D "_DLL" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_USRDLL" /FAcs /FD /c
# SUBTRACT BASE CPP /nologo /Ox /Ot /Oa /Ow /Oi /Gy /Fr /YX
# ADD CPP /MT /W3 /GR /GX /Zi /Og /Os /Ob2 /Gf /D "_MBCS" /D "_USRDLL" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FAcs /FD /c
# SUBTRACT CPP /nologo /Ox /Ot /Oa /Ow /Oi /Gy /Fr /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /o"q:/putlib/Release/putlib.bsc"
# SUBTRACT BASE BSC32 /nologo
# ADD BSC32 /o"q:/putlib/Release/putlib.bsc"
# SUBTRACT BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /subsystem:console /map:"c:\bin/putlib.map" /debug /machine:I386 /out:"c:\bin\\putlib.exe"
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 /subsystem:console /map:"c:\bin/putlibs.map" /debug /machine:I386 /nodefaultlib:"libcpmt.lib" /out:"c:\bin\\putlibs.exe"
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "putlib - Win32 Release"
# Name "putlib - Win32 Debug"
# Name "putlib - Win32 Static"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=K:\e\Prod\misc\putlib.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
