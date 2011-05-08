# Microsoft Developer Studio Project File - Name="libxml2_testSchemas" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=testSchemas - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libxml2_testSchemas.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libxml2_testSchemas.mak" CFG="testSchemas - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "testSchemas - Win32 DLL Unicode Release" (based on "Win32 (x86) Console Application")
!MESSAGE "testSchemas - Win32 DLL Unicode Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "testSchemas - Win32 DLL Release" (based on "Win32 (x86) Console Application")
!MESSAGE "testSchemas - Win32 DLL Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "testSchemas - Win32 Unicode Release" (based on "Win32 (x86) Console Application")
!MESSAGE "testSchemas - Win32 Unicode Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "testSchemas - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "testSchemas - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "testSchemas - Win32 DLL Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\bin"
# PROP BASE Intermediate_Dir "msvc6prj\testSchemas"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "msvc6prj\testSchemas"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_UNICODE" /D "NDEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_UNICODE" /D "NDEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x409 /d "_UNICODE" /i "..\include" /i "c:\iconv\include" /d "NDEBUG" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
# ADD RSC /l 0x409 /d "_UNICODE" /i "..\include" /i "c:\iconv\include" /d "NDEBUG" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /subsystem:console
# ADD LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /subsystem:console

!ELSEIF  "$(CFG)" == "testSchemas - Win32 DLL Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\bin"
# PROP BASE Intermediate_Dir "msvc6prj\testSchemas"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "msvc6prj\testSchemas"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_UNICODE" /D "_DEBUG" /D "_DEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_UNICODE" /D "_DEBUG" /D "_DEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x409 /d "_UNICODE" /d "_DEBUG" /i "..\include" /i "c:\iconv\include" /d "_DEBUG" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
# ADD RSC /l 0x409 /d "_UNICODE" /d "_DEBUG" /i "..\include" /i "c:\iconv\include" /d "_DEBUG" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /debug /subsystem:console
# ADD LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /debug /subsystem:console

!ELSEIF  "$(CFG)" == "testSchemas - Win32 DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\bin"
# PROP BASE Intermediate_Dir "msvc6prj\testSchemas"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "msvc6prj\testSchemas"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\bin\testSchemas.pdb /D "WIN32" /D "NDEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\bin\testSchemas.pdb /D "WIN32" /D "NDEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x409 /i "..\include" /i "c:\iconv\include" /d "NDEBUG" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
# ADD RSC /l 0x409 /i "..\include" /i "c:\iconv\include" /d "NDEBUG" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /subsystem:console
# ADD LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /subsystem:console

!ELSEIF  "$(CFG)" == "testSchemas - Win32 DLL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\bin"
# PROP BASE Intermediate_Dir "msvc6prj\testSchemas"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "msvc6prj\testSchemas"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_DEBUG" /D "_DEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_DEBUG" /D "_DEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x409 /d "_DEBUG" /i "..\include" /i "c:\iconv\include" /d "_DEBUG" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
# ADD RSC /l 0x409 /d "_DEBUG" /i "..\include" /i "c:\iconv\include" /d "_DEBUG" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /debug /subsystem:console
# ADD LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /debug /subsystem:console

!ELSEIF  "$(CFG)" == "testSchemas - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\bin"
# PROP BASE Intermediate_Dir "msvc6prj\testSchemas"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "msvc6prj\testSchemas"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_UNICODE" /D "NDEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_UNICODE" /D "NDEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x409 /d "_UNICODE" /i "..\include" /i "c:\iconv\include" /d "NDEBUG" /d "LIBXML_STATIC" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
# ADD RSC /l 0x409 /d "_UNICODE" /i "..\include" /i "c:\iconv\include" /d "NDEBUG" /d "LIBXML_STATIC" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /subsystem:console
# ADD LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /subsystem:console

!ELSEIF  "$(CFG)" == "testSchemas - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\bin"
# PROP BASE Intermediate_Dir "msvc6prj\testSchemas"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "msvc6prj\testSchemas"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_UNICODE" /D "_DEBUG" /D "_DEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_UNICODE" /D "_DEBUG" /D "_DEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x409 /d "_UNICODE" /d "_DEBUG" /i "..\include" /i "c:\iconv\include" /d "_DEBUG" /d "LIBXML_STATIC" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
# ADD RSC /l 0x409 /d "_UNICODE" /d "_DEBUG" /i "..\include" /i "c:\iconv\include" /d "_DEBUG" /d "LIBXML_STATIC" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /debug /subsystem:console
# ADD LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /debug /subsystem:console

!ELSEIF  "$(CFG)" == "testSchemas - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\bin"
# PROP BASE Intermediate_Dir "msvc6prj\testSchemas"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "msvc6prj\testSchemas"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\bin\testSchemas.pdb /D "WIN32" /D "NDEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\bin\testSchemas.pdb /D "WIN32" /D "NDEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x409 /i "..\include" /i "c:\iconv\include" /d "NDEBUG" /d "LIBXML_STATIC" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
# ADD RSC /l 0x409 /i "..\include" /i "c:\iconv\include" /d "NDEBUG" /d "LIBXML_STATIC" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /subsystem:console
# ADD LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /subsystem:console

!ELSEIF  "$(CFG)" == "testSchemas - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\bin"
# PROP BASE Intermediate_Dir "msvc6prj\testSchemas"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "msvc6prj\testSchemas"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_DEBUG" /D "_DEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\bin\testSchemas.pdb /D "WIN32" /D "_DEBUG" /D "_DEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x409 /d "_DEBUG" /i "..\include" /i "c:\iconv\include" /d "_DEBUG" /d "LIBXML_STATIC" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
# ADD RSC /l 0x409 /d "_DEBUG" /i "..\include" /i "c:\iconv\include" /d "_DEBUG" /d "LIBXML_STATIC" /d "_REENTRANT" /d "HAVE_WIN32_THREADS" /d "WIN32" /d "_WINDOWS" /d "_MBCS" /d _CONSOLE
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /debug /subsystem:console
# ADD LINK32 ..\lib\libxml2.lib iconv.lib wsock32.lib /nologo /machine:i386 /out:"..\bin\testSchemas.exe" /libpath:"c:\iconv\lib" /debug /subsystem:console

!ENDIF

# Begin Target

# Name "testSchemas - Win32 DLL Unicode Release"
# Name "testSchemas - Win32 DLL Unicode Debug"
# Name "testSchemas - Win32 DLL Release"
# Name "testSchemas - Win32 DLL Debug"
# Name "testSchemas - Win32 Unicode Release"
# Name "testSchemas - Win32 Unicode Debug"
# Name "testSchemas - Win32 Release"
# Name "testSchemas - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\..\testSchemas.c
# End Source File
# End Group
# End Target
# End Project

