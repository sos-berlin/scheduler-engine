# Microsoft Developer Studio Project File - Name="kram" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=kram - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "kram.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "kram.mak" CFG="kram - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "kram - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "kram - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kram - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\kram\Release"
# PROP BASE Intermediate_Dir ".\kram\Release"
# PROP BASE Target_Dir ".\kram"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ".\kram"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GR /GX /Og /Os /Ob2 /Gf /Gy /D "STRICT" /D "_MBCS" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FAcs /Yu"precomp.h" /FD /c
# SUBTRACT CPP /Ox /Ot /Oa /Ow /Oi /Fr
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\kram\Release"
# PROP BASE Intermediate_Dir ".\kram\Release"
# PROP BASE Target_Dir ".\kram"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ".\kram"
# ADD BASE CPP /G5 /MD /W3 /Gi /GR /GX /Zd /Og /Oi /Os /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /c
# SUBTRACT BASE CPP /nologo /Ox /Ot /Oa /Ow /Fr /YX /Yc /Yu
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GR /GX /Zi /Od /Gf /Gy /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /FR /Yu"precomp.h" /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"c:\lib32/kram.bsc"
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"c:\lib32\kram.lib"
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "kram - Win32 Release"
# Name "kram - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=A2a.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Area.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Assert.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Charset.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Chkarray.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Cobfield.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\com_server.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\com_simple_standards.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Decimal.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Delphi.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Dynobj.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=E2a.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Ebc2iso.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Ebcdic.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Ebcdifld.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\env.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Exp10.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Frmfield.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Iso2ebc.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Itoa.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Licence.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\licenceg.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Log.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Logwin.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=mfcstrng.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Msec.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Nullasgn.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\olereg.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\oleserv.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\olestd.CXX

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Optional.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Pointer.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Profile.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=sleep.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sos_mail.cxx
# End Source File
# Begin Source File

SOURCE=.\sos_mail_java.cxx
# End Source File
# Begin Source File

SOURCE=.\sos_mail_jmail.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sos_perl.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosalloc.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosarray.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosbeep.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosclien.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosctype.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosdate.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosdde.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosdll.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosdumcl.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\soserror.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosfact.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosfield.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosfiltr.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosfld2.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=sosfunc.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Soslimt2.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Soslimtx.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Soslist.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=sosmain0.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosmsg.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosmswin.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosnum.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosobj.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosobjba.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=sosole.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosopt.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosprof.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=sosprog.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sosscrpt.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosset.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossock.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossql1.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossql2.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sossql2a.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossql3.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossql4.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sossql_group_by.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossqlfl.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=SOSSQLFN.CXX

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossqlty.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosstat.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosstrea.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosstrg0.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosstrng.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossv.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossv2.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sossys.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sostimer.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sosuser.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Soswin.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Soswnmsg.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sosxml.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sqlfield.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sqlutil.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Stdfield.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Strlwr.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Strupr.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Subrange.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Svstring.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Sysdep.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\thread.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\thread_semaphore.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Truncsp.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Typereg.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Video.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Videowin.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Xception.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Xlat.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy-
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Xlatgerm.cxx

!IF  "$(CFG)" == "kram - Win32 Release"

# ADD CPP /Os /Oy- /Yu"precomp.h"
# SUBTRACT CPP /Ox /Ot

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# ADD CPP /Yu"precomp.h"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\com.h
# End Source File
# Begin Source File

SOURCE=.\com_server.h
# End Source File
# Begin Source File

SOURCE=.\env.h
# End Source File
# Begin Source File

SOURCE=kram.hrc
# End Source File
# Begin Source File

SOURCE=.\rtf_parser.h
# End Source File
# Begin Source File

SOURCE=.\sos_java.h
# End Source File
# Begin Source File

SOURCE=.\sos_perl.h
# End Source File
# Begin Source File

SOURCE=.\soserror.h
# End Source File
# Begin Source File

SOURCE=.\sosxml.h
# End Source File
# Begin Source File

SOURCE=.\thread.h
# End Source File
# Begin Source File

SOURCE=.\thread_data.h
# End Source File
# Begin Source File

SOURCE=.\thread_semaphore.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\xml_dom.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Java"

# PROP Default_Filter ".java"
# Begin Source File

SOURCE=.\sos\mail\Message.java

!IF  "$(CFG)" == "kram - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\sos\mail\Message.java
InputName=Message

"$(OutDir)/sos/mail/$(InputName).class" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) -sourcepath . -classpath $(CLASSPATH);$(OutDir) $(InputPath) 
	javac -d $(OutDir) -sourcepath . -classpath $(CLASSPATH);$(OutDir) $(InputPath) 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "kram - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\sos\mail\Message.java
InputName=Message

"$(OutDir)/sos/mail/$(InputName).class" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) -sourcepath . -classpath $(CLASSPATH);$(OutDir) $(InputPath) 
	javac -d $(OutDir) -sourcepath . -classpath $(CLASSPATH);$(OutDir) $(InputPath) 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=_precomp.cxx
# ADD CPP /Yc"precomp.h"
# End Source File
# End Target
# End Project
