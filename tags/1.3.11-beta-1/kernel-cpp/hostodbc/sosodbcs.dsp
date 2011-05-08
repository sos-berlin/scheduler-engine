# Microsoft Developer Studio Project File - Name="sosodbcs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=sosodbcs - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "sosodbcs.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "sosodbcs.mak" CFG="sosodbcs - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "sosodbcs - Win32 Release" (basierend auf  "Win32 (x86) Application")
!MESSAGE "sosodbcs - Win32 Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sosodbcs - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\sosodbcs\Release"
# PROP BASE Intermediate_Dir ".\sosodbcs\Release"
# PROP BASE Target_Dir ".\sosodbcs"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..bin"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\sosodbcs"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GR /GX /Zi /Og /Os /Ob2 /Gf /D "NDEBUG" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /FAcs /FD /c
# SUBTRACT CPP /Oi /Gy /Fr /YX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"release\sosodbcs.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ctl3d32s.lib delayimp.lib kernel32.lib user32.lib gdi32.lib version.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /pdb:"..\bin/sosodbcs.pdb" /map:"../bin/sosodbcs.map" /machine:I386 /nodefaultlib:"nafxcw.lib" /out:"..\bin\\sosodbcs.exe"
# SUBTRACT LINK32 /verbose /profile /pdb:none /incremental:yes /debug
# Begin Custom Build - gzip
InputPath=\prod\bin\sosodbcs.exe
SOURCE="$(InputPath)"

"$(InputPath).gz" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	gzip --name <$(InputPath) >$(InputPath).gz

# End Custom Build

!ELSEIF  "$(CFG)" == "sosodbcs - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\sosodbcs\Debug"
# PROP BASE Intermediate_Dir ".\sosodbcs\Debug"
# PROP BASE Target_Dir ".\sosodbcs"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\sosodbcs"
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GR /GX /Zi /Od /Gf /D "_DEBUG" /D "_MBCS" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /FR /FD /c
# SUBTRACT CPP /Gy
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 ctl3d32s.lib th32.lib delayimp.lib kernel32.lib user32.lib gdi32.lib version.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /pdb:"..\bind\sosodbcs.pdb" /debug /machine:I386 /nodefaultlib:"nafxcwd.lib" /out:"..\bind\\sosodbcs.exe"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "sosodbcs - Win32 Release"
# Name "sosodbcs - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=Drvsetup.c
# End Source File
# Begin Source File

SOURCE=Drvsetup.rc
# End Source File
# Begin Source File

SOURCE=sosodbc.rc
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=drvsetup.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\drvsetup.ico
# End Source File
# End Group
# End Target
# End Project
