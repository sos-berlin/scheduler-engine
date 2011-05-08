# Microsoft Developer Studio Project File - Name="sosodbci" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sosodbci - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "sosodbci.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "sosodbci.mak" CFG="sosodbci - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "sosodbci - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sosodbci - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sosodbci - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\sosodbci\Release"
# PROP BASE Intermediate_Dir ".\sosodbci\Release"
# PROP BASE Target_Dir ".\sosodbci"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "c:\bin"
# PROP Intermediate_Dir "q:\sosodbci\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\sosodbci"
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /MD /W3 /GR /GX /Zi /Og /Os /Ob2 /Gf /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_DLL" /D "STRICT" /FAcs /FD /c
# SUBTRACT CPP /nologo /Oi /Gy /Fr /YX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /o".\sosodbci\Release\osodbci.bsc"
# SUBTRACT BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ctl3d32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /subsystem:windows /dll /map:"c:\bin/sosodbci.map" /debug /machine:I386 /out:"c:\bin\\sosodbci.dll"
# SUBTRACT LINK32 /nologo /verbose /profile /incremental:yes

!ELSEIF  "$(CFG)" == "sosodbci - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\sosodbci\Debug"
# PROP BASE Intermediate_Dir ".\sosodbci\Debug"
# PROP BASE Target_Dir ".\sosodbci"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "q:\sosodbci\Debug"
# PROP Intermediate_Dir "q:\sosodbci\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ".\sosodbci"
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /MDd /W3 /Gm /Gi /GR /GX /ZI /Od /Ob1 /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "_DLL" /D "STRICT" /FD /c
# SUBTRACT CPP /nologo /WX /Fr /YX
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 ctl3d32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib th32.lib /subsystem:windows /dll /pdb:"q:\bin\sosodbci.pdb" /debug /machine:I386 /out:"q:\bin\\sosodbci.dll"
# SUBTRACT LINK32 /nologo /verbose /pdb:none /map

!ENDIF 

# Begin Target

# Name "sosodbci - Win32 Release"
# Name "sosodbci - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=K:\E\Prod\Sosodbc\Odbcdll.cxx
# End Source File
# Begin Source File

SOURCE=K:\e\Prod\Sosodbc\Odbcstup.cxx
# End Source File
# Begin Source File

SOURCE=K:\e\Prod\Sosodbc\sosodbci.def
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
