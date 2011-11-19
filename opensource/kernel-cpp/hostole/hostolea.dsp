# Microsoft Developer Studio Project File - Name="hostOLEa" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=hostOLEa - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "hostOLEa.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "hostOLEa.mak" CFG="hostOLEa - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "hostOLEa - Win32 Release" (basierend auf  "Win32 (x86) Application")
!MESSAGE "hostOLEa - Win32 Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hostOLEa - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\hostOLEa\Release"
# PROP BASE Intermediate_Dir ".\hostOLEa\Release"
# PROP BASE Target_Dir ".\hostOLEa"
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "q:\hostOLEa\Release"
# PROP Intermediate_Dir "q:\hostOLEa\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\hostOLEa"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GR /GX /Zi /Og /Os /Ob2 /Gf /D "_MBCS" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FAcs /FD /c
# SUBTRACT CPP /Gy /YX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 delayimp.lib libctleasy.lib /nologo /subsystem:windows /pdb:"q:/bin//hostOLEa.pdb" /map:"c:\bin\hostOLEa.map" /machine:I386 /nodefaultlib:"libcpmt.lib" /out:"q:\bin//hostOLEa.exe" /delayload:winspool.drv /delayload:libctleasy.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "hostOLEa - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\hostOLEa\Debug"
# PROP BASE Intermediate_Dir ".\hostOLEa\Debug"
# PROP BASE Target_Dir ".\hostOLEa"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "q:\hostOLEa\Debug"
# PROP Intermediate_Dir "q:\hostOLEa\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\hostOLEa"
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gi /GR /GX /Zi /Od /Ob1 /D "_DEBUG" /D "_AFXDLL" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /FR /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 th32.lib delayimp.lib libctleasy.lib /nologo /subsystem:windows /pdb:"q:\bind\hostOLEa.pdb" /debug /machine:I386 /out:"q:\bind\hostOLEa.exe" /delayload:libctleasy.dll
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "hostOLEa - Win32 Release"
# Name "hostOLEa - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\factory\factory.cxx
# End Source File
# Begin Source File

SOURCE=K:\e\prod\hostole\hostole.cxx
# End Source File
# Begin Source File

SOURCE=K:\e\prod\hostole\hostole.odl
# End Source File
# Begin Source File

SOURCE=K:\E\Prod\Hostole\Hostolea.cxx
# End Source File
# Begin Source File

SOURCE=K:\E\Prod\Hostole\Hostolea.rc
# End Source File
# Begin Source File

SOURCE=.\olereg.cxx
# End Source File
# Begin Source File

SOURCE=.\oleserv.cxx
# End Source File
# Begin Source File

SOURCE=K:\E\Prod\Kram\sosprog.cxx
# End Source File
# Begin Source File

SOURCE=K:\E\Prod\Kram\Soswnmai.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=K:\e\prod\hostole\hostole.h
# End Source File
# Begin Source File

SOURCE=K:\e\prod\hostole\hostole2.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=K:\e\PROD\HOSTOLE\HOSTOLE.HRC
# End Source File
# End Target
# End Project
