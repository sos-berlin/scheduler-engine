# Microsoft Developer Studio Project File - Name="file" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=file - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "file.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "file.mak" CFG="file - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "file - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "file - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "file - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\file\Release"
# PROP BASE Intermediate_Dir ".\file\Release"
# PROP BASE Target_Dir ".\file"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ".\file"
# ADD BASE CPP /G5 /MD /W3 /Gi /GR /GX /Zd /Og /Oi /Os /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /c
# SUBTRACT BASE CPP /nologo /Ox /Ot /Oa /Ow /Fr /YX /Yc /Yu
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GR /GX /Zi /Od /Gf /Gy /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /FR /Yu"precomp.h" /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"c:\lib32/file.bsc"
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"c:\lib32\file.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\file\Release"
# PROP BASE Intermediate_Dir ".\file\Release"
# PROP BASE Target_Dir ".\file"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ".\file"
# ADD BASE CPP /G5 /MTd /W4 /Gm /Gi /GR /GX /Zi /Od /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /c
# SUBTRACT BASE CPP /nologo /Fr /YX /Yc /Yu
# ADD CPP /nologo /MT /W3 /GR /GX /Og /Os /Ob2 /Gf /Gy /D "_MBCS" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FAcs /Yu"precomp.h" /FD /c
# SUBTRACT CPP /Ox /Ot /Oi /Fr
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"c:\lib32/file.bsc"
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"file.lib"
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "file - Win32 Debug"
# Name "file - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=Absfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Alias.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Anyfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cache_file.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Cachefl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=cachsqfl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clipbrd.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Colsfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Comfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Concatfl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Convfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\create_table_file.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Ddefile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Deletefl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dir.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Dosdir.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Ebcascfl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ebofile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Emukeyfl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\errfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Filebase.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Filedde.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Filekey.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Filetab.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Filter.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Fldfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Flstream.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\framedm.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Frmpage.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\head_file.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=inetfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Inlinefl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Insertfl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdbc.cxx
# End Source File
# Begin Source File

SOURCE=.\jmail_file.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Keyarray.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Keyfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Keyflarr.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Keyseqfl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=lockwait.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\logfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Mailfl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Mapifl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Memfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Nl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\nlfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=nullfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Objfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Odbc.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Odbctype.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=olefile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Oracle.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_file.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\printers.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Progfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Protfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Recfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Recnofil.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Rectab.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sam.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=SAM3FILE.CXX

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Selall.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosdb.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sqlfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Stdfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Storefl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sysfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Tabrec.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tail_file.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tb.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Teefl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=TYPEFILE.CXX

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Updatefl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Vflfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\winapifl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Wprtfile.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xml_file.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# Begin Source File

SOURCE=zlibfl.cxx

!IF  "$(CFG)" == "file - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ELSEIF  "$(CFG)" == "file - Win32 Release"

# ADD CPP /Os /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=DOSDIR.H
# End Source File
# Begin Source File

SOURCE=FRMPREV.H
# End Source File
# Begin Source File

SOURCE=PROTFILE.H
# End Source File
# Begin Source File

SOURCE=SYSFILE.H
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=_precomp.cxx
# ADD CPP /Yc"precomp.h"
# End Source File
# End Target
# End Project
