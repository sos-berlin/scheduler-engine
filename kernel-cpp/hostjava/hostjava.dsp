# Microsoft Developer Studio Project File - Name="hostjava" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=hostjava - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "hostjava.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "hostjava.mak" CFG="hostjava - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "hostjava - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hostjava - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hostjava - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HOSTJAVA_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /Oa /Ow /Og /Os /Ob2 /Gf /Gy /D "_MBCS" /D "NDEBUG" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /FAcs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x16000000" /dll /pdb:none /map /machine:I386 /out:"../bin/hostjava.dll"
# Begin Custom Build - echo cd $(OutDir) && jar cf $(TargetDir)/hostjava.jar sos/hostware
OutDir=.\Release
TargetDir=\prod\bin
InputPath=\prod\bin\hostjava.dll
SOURCE="$(InputPath)"

"$(TargetDir)/hostjava.jar" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd $(OutDir) && jar cf $(TargetDir)/hostjava.jar sos/hostware

# End Custom Build

!ELSEIF  "$(CFG)" == "hostjava - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HOSTJAVA_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GR /GX /Zi /Od /Gf /Gy /D "_DEBUG" /D "_MBCS" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x16000000" /dll /debug /machine:I386 /out:"../bind/hostjava.dll"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - echo cd $(OutDir) && jar cf $(TargetDir)/hostjava.jar sos/hostware
OutDir=.\Debug
TargetDir=\prod\bind
InputPath=\prod\bind\hostjava.dll
SOURCE="$(InputPath)"

"$(TargetDir)/hostjava.jar" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd $(OutDir) && jar cf $(TargetDir)/hostjava.jar sos/hostware

# End Custom Build

!ENDIF 

# Begin Target

# Name "hostjava - Win32 Release"
# Name "hostjava - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\hostjava.cxx
# End Source File
# Begin Source File

SOURCE=.\hostjava.rc
# End Source File
# Begin Source File

SOURCE=.\java_Factory_processor.cxx
# End Source File
# Begin Source File

SOURCE=.\java_File.cxx
# End Source File
# Begin Source File

SOURCE=.\java_Series_factory.cxx
# End Source File
# Begin Source File

SOURCE=.\java_Variable.cxx
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\hostjava.h
# End Source File
# Begin Source File

SOURCE=.\java_Factory_processor.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Java"

# PROP Default_Filter ".java"
# Begin Source File

SOURCE=.\sos\hostware\Factory_processor.java

!IF  "$(CFG)" == "hostjava - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\sos\hostware\Factory_processor.java
InputName=Factory_processor

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hostjava - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\sos\hostware\Factory_processor.java
InputName=Factory_processor

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sos\hostware\File.java

!IF  "$(CFG)" == "hostjava - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\sos\hostware\File.java
InputName=File

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hostjava - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\sos\hostware\File.java
InputName=File

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sos\hostware\Global.java

!IF  "$(CFG)" == "hostjava - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\sos\hostware\Global.java
InputName=Global

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hostjava - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\sos\hostware\Global.java
InputName=Global

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sos\hostware\Scripttext_flags.java

!IF  "$(CFG)" == "hostjava - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\sos\hostware\Scripttext_flags.java
InputName=Scripttext_flags

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hostjava - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\sos\hostware\Scripttext_flags.java
InputName=Scripttext_flags

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sos\hostware\Series_factory.java

!IF  "$(CFG)" == "hostjava - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hostjava - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\sos\hostware\Series_factory.java
InputName=Series_factory

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sos\hostware\Variable.java

!IF  "$(CFG)" == "hostjava - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\sos\hostware\Variable.java
InputName=Variable

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hostjava - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\sos\hostware\Variable.java
InputName=Variable

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sos\hostware\Variant.java

!IF  "$(CFG)" == "hostjava - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\sos\hostware\Variant.java
InputName=Variant

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hostjava - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\sos\hostware\Variant.java
InputName=Variant

"$(OutDir)/java.h/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo javac -d $(OutDir) $(InputPath) 
	javac -d $(OutDir) $(InputPath) 
	echo javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	javah -o $(OutDir)/java.h/$(InputName).h -classpath $(OutDir) sos.hostware.$(InputName) 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
