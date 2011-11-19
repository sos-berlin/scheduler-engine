@echo off
echo Dateien fÅr hostODBC (32-bit) werden zusammengestellt
echo ˇ

if errorlevel 1 rem

rem if "%1" == "" goto USAGE

cd k:\e\prod\sosodbc
j:
cd j:\e\setup\hostodbc
del j:\e\setup\hostodbc\*.*

echo > %TMP%\makedisk.log

rem 4DOS?
set makedisk_copy=copy /Y
if "%@ready[%_disk]" == "1"  set makedisk_copy=copy /Q
if "%OS%" == "Windows_NT"    set makedisk_copy=copy
set makedisk_del=del /Y
if "%@ready[%_disk]" == "1"  set makedisk_del=del /Q
if "%OS%" == "Windows_NT"    set makedisk_del=del

echo setup.exe      Setup: Baut das temporÑre Verzeichnis auf
%makedisk_copy% s:\ms\odbcsdk.210\redist32\setup.exe setup.exe
if errorlevel 1 goto ERROR

echo setup.lst      Setup: Liste der Dateien fÅr das temporÑre Verzeichnis
%makedisk_copy% k:\e\prod\sosodbc\setup.lst > %TMP%\makedisk.log
if errorlevel 1 goto ERROR

echo _bootstp.exe   Setup
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\_bootstp.exe
if errorlevel 1 goto ERROR

echo _mssetup.exe   Setup
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\_mssetup.ex_
if errorlevel 1 goto ERROR

echo readme.txt
%makedisk_copy% k:\e\prod\sosodbc\readme.txt
if errorlevel 1 goto ERROR

echo odbc.inf       Installationsanleitung fÅr ODBC
%makedisk_copy% k:\e\prod\sosodbc\odbc.inf
if errorlevel 1 goto ERROR

echo odbc.ini       FÅr Standard-Datenquelle
%makedisk_copy% k:\e\prod\sosodbc\odbc.ini
if errorlevel 1 goto ERROR

echo odbcinst.ini   FÅr Standard-Datenquelle
%makedisk_copy% k:\e\prod\sosodbc\odbcinst.ini
if errorlevel 1 goto ERROR

echo drvsetup.exe   hostODBC-Installation
s:\bc4\bin\compress c:\bin\sosodbcs.exe drvsetup.ex_  >  %TMP%\makedisk.log
if errorlevel 1 goto ERROR

rem echo sosodbci.dll   Zur Konfiguration der Datenquellen (setup=sosodbci.dll)
rem s:\bc4\bin\compress c:\bin\sosodbci.dll sosodbci.dl_  >  %TMP%\makedisk.log
rem if errorlevel 1 goto ERROR

echo sosodbc.dll    Der ODBC-Treiber
s:\bc4\bin\compress c:\bin\sosodbc.dll sosodbc.dl_  >  %TMP%\makedisk.log
if errorlevel 1 goto ERROR


echo soserror.txt   Texte zu unseren Fehlercodes
s:\bc4\bin\compress k:\e\prod\kram\soserror.txt soserror.tx_  >  %TMP%\makedisk.log
if errorlevel 1 goto ERROR

echo sosodbc.cob    Beispiel fÅr einen BS2000-Katalog
%makedisk_copy% k:\e\prod\sosodbc\sosodbc.cob
if errorlevel 1 goto ERROR


rem echo msvcrt40.dll   Microsoft Visual C++ Runtime 4.0 DLL fÅr sosodbc.dll u.a.
rem s:\bc4\bin\compress s:\ms\msdev\redist\msvcrt40.dll msvcrt40.dl_  >  %TMP%\makedisk.log
rem if errorlevel 1 goto ERROR

echo msvcrt.dll     Microsoft Visual C Runtime DLL fÅr sosodbc.dll u.a.
rem 4.2 s:\bc4\bin\compress s:\ms\msdev\redist\msvcrt.dll msvcrt.dl_  >  %TMP%\makedisk.log
s:\bc4\bin\compress s:\ms\redist\msvcrt.dll msvcrt.dl_  >  %TMP%\makedisk.log
if errorlevel 1 goto ERROR

echo msvcirt.dll    Microsoft Visual C++ iostream DLL fÅr sosodbc.dll u.a.
rem 4.2 s:\bc4\bin\compress s:\ms\msdev\redist\msvcirt.dll msvcirt.dl_  >  %TMP%\makedisk.log
s:\bc4\bin\compress s:\ms\redist\msvcirt.dll msvcirt.dl_  >  %TMP%\makedisk.log
if errorlevel 1 goto ERROR

echo mfc42.dll      Microsoft Foundation Classes 4.2 fÅr sosodbc.dll u.a.
rem 4.2 s:\bc4\bin\compress s:\ms\msdev\redist\mfc42.dll mfc42.dl_  >  %TMP%\makedisk.log
s:\bc4\bin\compress s:\ms\redist\mfc42.dll mfc42.dl_  >  %TMP%\makedisk.log
if errorlevel 1 goto ERROR


echo ctl3d32.dll    3D-Controls f¸r Windows 95
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\ctl3d95.dl_
if errorlevel 1 goto ERROR

echo ctl3d32.dll    3D-Controls f¸r Windows NT
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\ctl3dnt.dl_
if errorlevel 1 goto ERROR


echo odbccp32.cpl   ODBC Konfiguration, Datei fÅr Systemsteuerung
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\odbccp32.cp_
if errorlevel 1 goto ERROR

echo odbcad32.exe   ODBC Datenquellenverwalter
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\odbcad32.ex_
if errorlevel 1 goto ERROR

echo odbccp32.dll   ODBC Konfiguration
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\odbccp32.dl_  > %TMP%\makedisk.log
if errorlevel 1 goto ERROR

echo odbcinst.hlp   ODBC Hilfe fÅr Konfiguration
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\odbcinst.hl_
if errorlevel 1 goto ERROR


echo odbc32.dll     ODBC Manager
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\odbc32.dl_
if errorlevel 1 goto ERROR

echo Passt nicht auf die Diskette, solange MFC benˆtigt wird:
echo odbccr32.dll   ODBC Cursor Library
rem %makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\odbccr32.dl_
rem if errorlevel 1 goto ERROR


echo odbc16gt.dll   ODBC 16bit generic thunking 16 to 32
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\odbc16gt.dl_
if errorlevel 1 goto ERROR

echo odbc32gt.dll   ODBC 32bit generic thunking 16 to 32
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\odbc32gt.dl_
if errorlevel 1 goto ERROR

echo ds16gt.dll     ODBC 16bit setup generic thunking 16 to 32
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\ds16gt.dl_
if errorlevel 1 goto ERROR

echo ds32gt.dll     ODBC 32bit setup generic thunking 16 to 32
%makedisk_copy% s:\ms\odbcsdk.210\drvsetup.kit\setup32\ds32gt.dl_
if errorlevel 1 goto ERROR


rem Folgende vier 16-bit-DLLs sind auch auf der hostODBC (16 bit)-Diskette!

rem echo odbc.dll       ODBC 16bit Driver Manager
rem %makedisk_copy% w:\ms\odbcsdk.200\drvsetup.kit\setup16\odbc.dl_
rem if errorlevel 1 goto ERROR

rem echo odbccurs.dll   ODBC 16bit cursor library
rem %makedisk_copy% s:\ms\odbcsdk.200\drvsetup.kit\setup16\odbccurs.dl_
rem if errorlevel 1 goto ERROR

rem echo odbcadm.exe    ODBC Datenquellenverwalter
rem %makedisk_copy% s:\ms\odbcsdk.200\drvsetup.kit\setup16\odbcadm.ex_
rem if errorlevel 1 goto ERROR

rem echo odbcinst.dll   ODBC Konfiguration
rem %makedisk_copy% s:\ms\odbcsdk.200\drvsetup.kit\setup16\odbcinst.dl_
rem if errorlevel 1 goto ERROR



set makedisk_copy=

goto ENDE

:ERROR
echo Fehler aufgetreten:
more < %TMP%\makedisk.log
goto :ENDE

:USAGE
echo usage: makedisk FROMDIR
echo ˇ
echo Kopiert die Dateien fÅr die ODBC-Installationsdiskette ins aktuelle
echo Verzeichnis.
echo sosodbc.dll, sosodbci.dll und sosodbcs.exe werden aus FROMDIR Åbernommen.
echo setup.lst, odbc.inf, odbc.ini, odbcinst.ini und soserror.txt werden
echo direkt aus k:\e\prod\sosodbc Åbernommen

:ENDE
