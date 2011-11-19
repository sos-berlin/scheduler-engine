# Microsoft Developer Studio Project File - Name="zschimmer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=zschimmer - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "zschimmer.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "zschimmer.mak" CFG="zschimmer - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "zschimmer - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "zschimmer - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "zschimmer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /Og /Os /Ob2 /Gf /Gy /I "..\libxml2\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FAcs /FD /c
# SUBTRACT CPP /Ox /Ot /YX /Yc /Yu
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "zschimmer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GR /GX /Zi /Od /Gf /Gy /I "..\libxml2\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "zschimmer - Win32 Release"
# Name "zschimmer - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\check_compiler.cxx
# End Source File
# Begin Source File

SOURCE=.\com.cxx
# End Source File
# Begin Source File

SOURCE=.\com_remote.cxx
# End Source File
# Begin Source File

SOURCE=.\file.cxx
# End Source File
# Begin Source File

SOURCE=.\java.cxx
# End Source File
# Begin Source File

SOURCE=.\log.cxx
# End Source File
# Begin Source File

SOURCE=.\regex_class.cxx
# End Source File
# Begin Source File

SOURCE=.\rtf\rtf.cxx
# End Source File
# Begin Source File

SOURCE=.\rtf\rtf_doc.cxx
# End Source File
# Begin Source File

SOURCE=.\rtf\rtf_parser.cxx
# End Source File
# Begin Source File

SOURCE=.\threads.cxx
# End Source File
# Begin Source File

SOURCE=.\utf8.cxx
# End Source File
# Begin Source File

SOURCE=.\xml_libxml2.cxx
# End Source File
# Begin Source File

SOURCE=.\xml_msxml.cxx
# End Source File
# Begin Source File

SOURCE=.\z_com.cxx
# End Source File
# Begin Source File

SOURCE=.\z_com_server.cxx
# End Source File
# Begin Source File

SOURCE=.\z_java.cxx
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\z_windows.cxx
# End Source File
# Begin Source File

SOURCE=.\zschimmer.cxx
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\base.h
# End Source File
# Begin Source File

SOURCE=.\com_remote.h
# End Source File
# Begin Source File

SOURCE=.\dom_libxml.h
# End Source File
# Begin Source File

SOURCE=.\event_base.h
# End Source File
# Begin Source File

SOURCE=.\file.h
# End Source File
# Begin Source File

SOURCE=.\rtf\headers.h
# End Source File
# Begin Source File

SOURCE=.\java.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\mutex.h
# End Source File
# Begin Source File

SOURCE=.\mutex_base.h
# End Source File
# Begin Source File

SOURCE=.\regex_class.h
# End Source File
# Begin Source File

SOURCE=.\rtf\rtf.h
# End Source File
# Begin Source File

SOURCE=.\rtf\rtf_doc.h
# End Source File
# Begin Source File

SOURCE=.\rtf\rtf_parser.h
# End Source File
# Begin Source File

SOURCE=.\xml\sax.h
# End Source File
# Begin Source File

SOURCE=.\system.h
# End Source File
# Begin Source File

SOURCE=.\thread_base.h
# End Source File
# Begin Source File

SOURCE=.\threads_base.h
# End Source File
# Begin Source File

SOURCE=.\utf8.h
# End Source File
# Begin Source File

SOURCE=.\xml.h
# End Source File
# Begin Source File

SOURCE=.\xml_libxml2.h
# End Source File
# Begin Source File

SOURCE=.\z_com.h
# End Source File
# Begin Source File

SOURCE=.\z_java.h
# End Source File
# Begin Source File

SOURCE=.\z_posix_mutex.h
# End Source File
# Begin Source File

SOURCE=.\z_windows.h
# End Source File
# Begin Source File

SOURCE=.\z_windows_mutex.h
# End Source File
# Begin Source File

SOURCE=.\zschimmer.h
# End Source File
# End Group
# Begin Group "Java sos.zschimmer.com"

# PROP Default_Filter "*.java"
# Begin Source File

SOURCE=.\Idispatch.java
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Target
# End Project
