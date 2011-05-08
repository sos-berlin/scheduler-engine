# Microsoft Developer Studio Project File - Name="hostOLE" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=hostOLE - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "hostOLE.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "hostOLE.mak" CFG="hostOLE - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "hostOLE - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hostOLE - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hostOLE - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\hostOLE\Release"
# PROP BASE Intermediate_Dir ".\hostOLE\Release"
# PROP BASE Target_Dir ".\hostOLE"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\hostOLE"
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GR /GX /Og /Os /Ob2 /Gf /Gy /D "_MBCS" /D "NDEBUG" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /FAcs /FD /c
# SUBTRACT CPP /Z<none> /Ox /Fr /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ../misc/lib/libctleasy.lib delayimp.lib kernel32.lib user32.lib gdi32.lib version.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../misc/lib/gsdll32.lib /nologo /base:"0x17000000" /subsystem:windows /dll /pdb:none /map:"..\bin\hostOLE.map" /machine:I386 /nodefaultlib:"nafxcw.lib" /def:".\Hostole.def" /out:"..\bin//hostOLE.dll" /delayload:winspool.drv /delayload:wsock32.dll /delayload:odbc32.dll /delayload:libctleasy.dll /delayload:gsdll32.dll
# Begin Custom Build - gzip
InputPath=\prod\bin\hostOLE.dll
SOURCE="$(InputPath)"

"$(InputPath).gz" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	gzip --name <$(InputPath) >$(InputPath).gz

# End Custom Build

!ELSEIF  "$(CFG)" == "hostOLE - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\hostOLE\Debug"
# PROP BASE Intermediate_Dir ".\hostOLE\Debug"
# PROP BASE Target_Dir ".\hostOLE"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\hostOLE"
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GR /GX /Zi /Od /Gf /Gy /D "_DEBUG" /D "_MBCS" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 th32.lib ../misc/lib/libctleasy.lib delayimp.lib kernel32.lib user32.lib gdi32.lib version.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../misc/lib/gsdll32.lib /nologo /base:"0x17000000" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"nafxcwd.lib" /def:".\Hostole.def" /out:"..\bind\\hostOLE.dll" /delayload:winspool.drv /delayload:wsock32.dll /delayload:odbc32.dll /delayload:libctleasy.dll /delayload:gsdll32.dll
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "hostOLE - Win32 Release"
# Name "hostOLE - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\exception.cxx
# End Source File
# Begin Source File

SOURCE=.\ghostscript.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=hostole.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=hostoled.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\mt940.cxx
# End Source File
# Begin Source File

SOURCE=.\ole_parser.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\script_object.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\variables.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\word_application.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=hostole.h
# End Source File
# Begin Source File

SOURCE=hostole2.h
# End Source File
# Begin Source File

SOURCE=.\hostole_odl.h
# End Source File
# Begin Source File

SOURCE=.\hostole_ptr.h
# End Source File
# Begin Source File

SOURCE=.\hostole_version.h
# End Source File
# Begin Source File

SOURCE=.\olereg.h
# End Source File
# Begin Source File

SOURCE=.\variables.h
# End Source File
# Begin Source File

SOURCE=.\word_application.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=Hostole.rc
# End Source File
# End Group
# Begin Group "Factory Processor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\factory\processor\factory_processor.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=..\factory\processor\factory_processor.h
# End Source File
# Begin Source File

SOURCE=..\factory\processor\factory_rtf.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=..\factory\processor\factory_rtf.h
# End Source File
# Begin Source File

SOURCE=..\factory\processor\factory_text.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=..\factory\processor\factory_text.h
# End Source File
# Begin Source File

SOURCE=..\factory\processor\series_factory.cxx
# ADD CPP /Yu"precomp.h"
# End Source File
# End Group
# Begin Source File

SOURCE=.\_precomp.cxx
# ADD CPP /Yc"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\Hostole.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=HOSTOLE.HRC
# End Source File
# Begin Source File

SOURCE=.\hostole.odl
# End Source File
# End Target
# End Project
