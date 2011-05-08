# Microsoft Developer Studio Project File - Name="libxml2_libxml2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libxml2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libxml2_libxml2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libxml2_libxml2.mak" CFG="libxml2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libxml2 - Win32 DLL Unicode Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libxml2 - Win32 DLL Unicode Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libxml2 - Win32 DLL Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libxml2 - Win32 DLL Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libxml2 - Win32 Unicode Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libxml2 - Win32 Unicode Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libxml2 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libxml2 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libxml2 - Win32 DLL Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\lib"
# PROP BASE Intermediate_Dir "msvc6prj\libxml2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib"
# PROP Intermediate_Dir "msvc6prj\libxml2"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_UNICODE" /D "NDEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_UNICODE" /D "NDEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libxml2.lib"
# ADD LIB32 /nologo /out:"..\lib\libxml2.lib"

!ELSEIF  "$(CFG)" == "libxml2 - Win32 DLL Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\lib"
# PROP BASE Intermediate_Dir "msvc6prj\libxml2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib"
# PROP Intermediate_Dir "msvc6prj\libxml2"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_UNICODE" /D "_DEBUG" /D "_DEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_UNICODE" /D "_DEBUG" /D "_DEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libxml2.lib"
# ADD LIB32 /nologo /out:"..\lib\libxml2.lib"

!ELSEIF  "$(CFG)" == "libxml2 - Win32 DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\lib"
# PROP BASE Intermediate_Dir "msvc6prj\libxml2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib"
# PROP Intermediate_Dir "msvc6prj\libxml2"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "NDEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "NDEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libxml2.lib"
# ADD LIB32 /nologo /out:"..\lib\libxml2.lib"

!ELSEIF  "$(CFG)" == "libxml2 - Win32 DLL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\lib"
# PROP BASE Intermediate_Dir "msvc6prj\libxml2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib"
# PROP Intermediate_Dir "msvc6prj\libxml2"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_DEBUG" /D "_DEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_DEBUG" /D "_DEBUG" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libxml2.lib"
# ADD LIB32 /nologo /out:"..\lib\libxml2.lib"

!ELSEIF  "$(CFG)" == "libxml2 - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\lib"
# PROP BASE Intermediate_Dir "msvc6prj\libxml2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib"
# PROP Intermediate_Dir "msvc6prj\libxml2"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_UNICODE" /D "NDEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_UNICODE" /D "NDEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libxml2.lib"
# ADD LIB32 /nologo /out:"..\lib\libxml2.lib"

!ELSEIF  "$(CFG)" == "libxml2 - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\lib"
# PROP BASE Intermediate_Dir "msvc6prj\libxml2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib"
# PROP Intermediate_Dir "msvc6prj\libxml2"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_UNICODE" /D "_DEBUG" /D "_DEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_UNICODE" /D "_DEBUG" /D "_DEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libxml2.lib"
# ADD LIB32 /nologo /out:"..\lib\libxml2.lib"

!ELSEIF  "$(CFG)" == "libxml2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\lib"
# PROP BASE Intermediate_Dir "msvc6prj\libxml2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib"
# PROP Intermediate_Dir "msvc6prj\libxml2"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "NDEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /w /O2 /I "..\include" /I "c:\iconv\include" /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "NDEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libxml2.lib"
# ADD LIB32 /nologo /out:"..\lib\libxml2.lib"

!ELSEIF  "$(CFG)" == "libxml2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\lib"
# PROP BASE Intermediate_Dir "msvc6prj\libxml2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib"
# PROP Intermediate_Dir "msvc6prj\libxml2"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_DEBUG" /D "_DEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /W4 /Od /I "..\include" /I "c:\iconv\include" /Zi /Gm /GZ /Fd..\lib\libxml2.pdb /D "WIN32" /D "_LIB" /D "_DEBUG" /D "_DEBUG" /D "LIBXML_STATIC" /D "_REENTRANT" /D "HAVE_WIN32_THREADS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libxml2.lib"
# ADD LIB32 /nologo /out:"..\lib\libxml2.lib"

!ENDIF

# Begin Target

# Name "libxml2 - Win32 DLL Unicode Release"
# Name "libxml2 - Win32 DLL Unicode Debug"
# Name "libxml2 - Win32 DLL Release"
# Name "libxml2 - Win32 DLL Debug"
# Name "libxml2 - Win32 Unicode Release"
# Name "libxml2 - Win32 Unicode Debug"
# Name "libxml2 - Win32 Release"
# Name "libxml2 - Win32 Debug"
# Begin Group "Config headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\..\include\win32config.h

!IF  "$(CFG)" == "libxml2 - Win32 DLL Unicode Release"

# Begin Custom Build - Creating the configuration file ..\config.h from ..\include\win32\win32config.h
InputPath=..\include\win32config.h

"..\include\win32config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libxml2 - Win32 DLL Unicode Debug"

# Begin Custom Build - Creating the configuration file ..\config.h from ..\include\win32\win32config.h
InputPath=..\include\win32config.h

"..\include\win32config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libxml2 - Win32 DLL Release"

# Begin Custom Build - Creating the configuration file ..\config.h from ..\include\win32\win32config.h
InputPath=..\include\win32config.h

"..\include\win32config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libxml2 - Win32 DLL Debug"

# Begin Custom Build - Creating the configuration file ..\config.h from ..\include\win32\win32config.h
InputPath=..\include\win32config.h

"..\include\win32config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libxml2 - Win32 Unicode Release"

# Begin Custom Build - Creating the configuration file ..\config.h from ..\include\win32\win32config.h
InputPath=..\include\win32config.h

"..\include\win32config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libxml2 - Win32 Unicode Debug"

# Begin Custom Build - Creating the configuration file ..\config.h from ..\include\win32\win32config.h
InputPath=..\include\win32config.h

"..\include\win32config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libxml2 - Win32 Release"

# Begin Custom Build - Creating the configuration file ..\config.h from ..\include\win32\win32config.h
InputPath=..\include\win32config.h

"..\include\win32config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libxml2 - Win32 Debug"

# Begin Custom Build - Creating the configuration file ..\config.h from ..\include\win32\win32config.h
InputPath=..\include\win32config.h

"..\include\win32config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\config.h

# End Custom Build

!ENDIF

# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\..\DOCBparser.c
# End Source File
# Begin Source File

SOURCE=.\..\HTMLparser.c
# End Source File
# Begin Source File

SOURCE=.\..\HTMLtree.c
# End Source File
# Begin Source File

SOURCE=.\..\SAX.c
# End Source File
# Begin Source File

SOURCE=.\..\SAX2.c
# End Source File
# Begin Source File

SOURCE=.\..\c14n.c
# End Source File
# Begin Source File

SOURCE=.\..\catalog.c
# End Source File
# Begin Source File

SOURCE=.\..\chvalid.c
# End Source File
# Begin Source File

SOURCE=.\..\debugXML.c
# End Source File
# Begin Source File

SOURCE=.\..\dict.c
# End Source File
# Begin Source File

SOURCE=.\..\encoding.c
# End Source File
# Begin Source File

SOURCE=.\..\entities.c
# End Source File
# Begin Source File

SOURCE=.\..\error.c
# End Source File
# Begin Source File

SOURCE=.\..\globals.c
# End Source File
# Begin Source File

SOURCE=.\..\hash.c
# End Source File
# Begin Source File

SOURCE=.\..\legacy.c
# End Source File
# Begin Source File

SOURCE=.\..\list.c
# End Source File
# Begin Source File

SOURCE=.\..\nanoftp.c
# End Source File
# Begin Source File

SOURCE=.\..\nanohttp.c
# End Source File
# Begin Source File

SOURCE=.\..\parser.c
# End Source File
# Begin Source File

SOURCE=.\..\parserInternals.c
# End Source File
# Begin Source File

SOURCE=.\..\pattern.c
# End Source File
# Begin Source File

SOURCE=.\..\relaxng.c
# End Source File
# Begin Source File

SOURCE=.\..\threads.c
# End Source File
# Begin Source File

SOURCE=.\..\tree.c
# End Source File
# Begin Source File

SOURCE=.\..\uri.c
# End Source File
# Begin Source File

SOURCE=.\..\valid.c
# End Source File
# Begin Source File

SOURCE=.\..\xinclude.c
# End Source File
# Begin Source File

SOURCE=.\..\xlink.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlIO.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlmemory.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlreader.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlregexp.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlsave.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlschemas.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlschemastypes.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlstring.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlunicode.c
# End Source File
# Begin Source File

SOURCE=.\..\xmlwriter.c
# End Source File
# Begin Source File

SOURCE=.\..\xpath.c
# End Source File
# Begin Source File

SOURCE=.\..\xpointer.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\..\include\libxml\DOCBparser.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\HTMLparser.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\HTMLtree.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\SAX.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\SAX2.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\c14n.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\catalog.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\chvalid.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\debugXML.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\dict.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\encoding.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\entities.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\globals.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\hash.h
# End Source File
# Begin Source File

SOURCE=.\..\libxml.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\list.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\nanoftp.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\nanohttp.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\parser.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\parserInternals.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\pattern.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\relaxng.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\schemasInternals.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\threads.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\tree.h
# End Source File
# Begin Source File

SOURCE=.\..\triodef.h
# End Source File
# Begin Source File

SOURCE=.\..\trionan.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\uri.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\valid.h
# End Source File
# Begin Source File

SOURCE=.\..\include\wsockcompat.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xinclude.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xlink.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlIO.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlautomata.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlerror.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlexports.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlmemory.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlmodule.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlreader.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlregexp.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlsave.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlschemas.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlschemastypes.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlstring.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlunicode.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlversion.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xmlwriter.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xpath.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xpathInternals.h
# End Source File
# Begin Source File

SOURCE=.\..\include\libxml\xpointer.h
# End Source File
# End Group
# End Target
# End Project

