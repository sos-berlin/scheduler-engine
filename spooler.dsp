# Microsoft Developer Studio Project File - Name="spooler" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=spooler - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "spooler.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "spooler.mak" CFG="spooler - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "spooler - Win32 Release" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE "spooler - Win32 Debug" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "spooler - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /Og /Os /Oy /Ob2 /Gf /Gy /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FAcs /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ../misc/lib/libctleasy.lib delayimp.lib kernel32.lib user32.lib gdi32.lib version.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /pdb:none /map:"../bin/spooler.map" /machine:I386 /out:"../bin/spooler.exe" /delayload:winspool.drv /delayload:wsock32.dll /delayload:ole32.dll /delayload:oleaut32.dll /delayload:advapi32.dll /delayload:libctleasy.dll
# Begin Custom Build - gzip
InputPath=\prod\bin\spooler.exe
SOURCE="$(InputPath)"

"$(InputPath).gz" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	gzip --name <$(InputPath) >$(InputPath).gz

# End Custom Build

!ELSEIF  "$(CFG)" == "spooler - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GR /GX /Zi /Od /Gf /Gy /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 th32.lib ../misc/lib/libctleasy.lib delayimp.lib kernel32.lib user32.lib gdi32.lib version.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /pdb:"../bind/spooler.pdb" /debug /machine:I386 /out:"../bind/spooler.exe" /delayload:winspool.drv /delayload:wsock32.dll /delayload:ole32.dll /delayload:oleaut32.dll /delayload:advapi32.dll /delayload:libctleasy.dll
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "spooler - Win32 Release"
# Name "spooler - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\spooler.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_com.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_command.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_communication.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_config.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_history.cxx
# End Source File
# Begin Source File

SOURCE=.\spooler_log.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_mail.cxx
# End Source File
# Begin Source File

SOURCE=.\spooler_script.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_security.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_service.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_task.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_thread.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_time.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# Begin Source File

SOURCE=.\spooler_wait.cxx
# ADD CPP /YX"spooler.h"
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\spooler.h
# End Source File
# Begin Source File

SOURCE=.\spooler.odl

!IF  "$(CFG)" == "spooler - Win32 Release"

# Begin Custom Build - midl /nologo /error all /out $(OutDir) $(InputPath)
OutDir=.\Release
InputPath=.\spooler.odl

"$(OutDir)/spooler.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /nologo /error all /out $(OutDir) $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "spooler - Win32 Debug"

# Begin Custom Build - midl /nologo /error all /out $(OutDir) $(InputPath)
OutDir=.\Debug
InputPath=.\spooler.odl

"$(OutDir)/spooler.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /nologo /error all /out $(OutDir) $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\spooler_com.h
# End Source File
# Begin Source File

SOURCE=.\spooler_command.h
# End Source File
# Begin Source File

SOURCE=.\spooler_common.h
# End Source File
# Begin Source File

SOURCE=.\spooler_communication.h
# End Source File
# Begin Source File

SOURCE=.\spooler_history.h
# End Source File
# Begin Source File

SOURCE=.\spooler_log.h
# End Source File
# Begin Source File

SOURCE=.\spooler_mail.h
# End Source File
# Begin Source File

SOURCE=.\spooler_script.h
# End Source File
# Begin Source File

SOURCE=.\spooler_security.h
# End Source File
# Begin Source File

SOURCE=.\spooler_service.h
# End Source File
# Begin Source File

SOURCE=.\spooler_task.h
# End Source File
# Begin Source File

SOURCE=.\spooler_thread.h
# End Source File
# Begin Source File

SOURCE=.\spooler_time.h
# End Source File
# Begin Source File

SOURCE=.\spooler_version.h
# End Source File
# Begin Source File

SOURCE=.\spooler_wait.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\spooler.rc
# End Source File
# End Group
# Begin Group "XML"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\spooler.dtd
# End Source File
# Begin Source File

SOURCE=.\spooler.xml
# End Source File
# End Group
# Begin Source File

SOURCE=..\kram\sosmain.cxx
# End Source File
# End Target
# End Project
