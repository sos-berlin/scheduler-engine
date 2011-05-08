@echo off
echo Dateien fÅr hostODBC (16-bit) werden zusammengestellt
echo .

rem if "%1" == "" goto USAGE

cd k:\e\prod\sosodbc
j:
cd j:\e\setup\hostdb16
del j:\e\setup\hostdb16\*.*

if exist odbc16.inf  goto QUELL_LAUFWERK

echo > %TMP%\makedisk.log

rem 4DOS?
set makedisk_copy=copy /Y
if "%@ready[%_disk]" == "1"  set makedisk_copy=copy /Q
if "%OS%" == "Windows_NT"    set makedisk_copy=copy
set makedisk_del=del /Y
if "%@ready[%_disk]" == "1"  set makedisk_del=del /Q
if "%OS%" == "Windows_NT"    set makedisk_del=del


echo setup.exe      Setup: Baut das temporÑre Verzeichnis auf
%makedisk_copy% s:\ms\odbcsdk.200\redist16\setup.exe setup.exe
if errorlevel 1 goto ERROR

echo setup.lst      Setup: Liste der Dateien fÅr das temporÑre Verzeichnis
%makedisk_copy% k:\e\prod\sosodbc\setup16.lst setup.lst > %TMP%\makedisk.log
if errorlevel 1 goto ERROR

echo _bootstp.exe   Setup
%makedisk_copy% s:\ms\odbcsdk.200\drvsetup.kit\setup16\_bootstp.exe
if errorlevel 1 goto ERROR

echo _mssetup.exe   Setup
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\_mssetup.ex_
if errorlevel 1 goto ERROR


echo readme.txt
%makedisk_copy% k:\e\prod\sosodbc\readme.txt
if errorlevel 1 goto ERROR

echo odbc.inf       Installationsanleitung fÅr ODBC
%makedisk_copy% k:\e\prod\sosodbc\odbc16.inf odbc.inf
if errorlevel 1 goto ERROR

echo odbc.ini       FÅr Standard-Datenquelle
%makedisk_copy% k:\e\prod\sosodbc\odbc.ini
if errorlevel 1 goto ERROR

echo odbcinst.ini   FÅr Standard-Datenquelle
%makedisk_copy% k:\e\prod\sosodbc\odbcinst.ini
if errorlevel 1 goto ERROR


echo drvsetup.exe   hostODBC-Installation
%makedisk_copy%  c:\bin\sosdb16s.exe %TMP%\
s:\bc4\bin\tdstrip %TMP%\sosdb16s.exe
compress %TMP%\sosdb16s.exe drvsetup.ex_ > %TMP%\makedisk.log
if errorlevel 1 goto ERROR
%makedisk_del% %TMP%\sosdb16s.exe
if errorlevel 1 rem egal

rem echo sosdb16i.dll   Zur Konfiguration der Datenquellen (setup=sosdb16i.dll)
rem %makedisk_copy% c:\bin\sosdb16i.dll %TMP%\
rem s:\bc4\bin\tdstrip %TMP%\sosdb16i.dll
rem compress %TMP%\sosdb16i.dll sosdb16i.dl_ > %TMP%\makedisk.log
rem if errorlevel 1 goto ERROR
rem %makedisk_del% %TMP%\sosdb16i.dll
rem if errorlevel 1 rem egal

echo sosdb16.dll    Der ODBC-Treiber
%makedisk_copy% c:\bin\sosdb16.dll %TMP%\
s:\bc4\bin\tdstrip %TMP%\sosdb16.dll
compress %TMP%\sosdb16.dll sosdb16.dl_ > %TMP%\makedisk.log
if errorlevel 1 goto ERROR
%makedisk_del% %TMP%\sosdb16.dll
if errorlevel 1 rem egal


echo soserror.txt   Texte zu unseren Fehlercodes
compress k:\e\prod\kram\soserror.txt soserror.tx_ > %TMP%\makedisk.log
if errorlevel 1 goto ERROR

echo sosodbc.cob    Beispiel fÅr einen BS2000-Katalog (im Windows-Systemverzeichnis?)
%makedisk_copy% k:\e\prod\sosodbc\sosodbc.cob
if errorlevel 1 goto ERROR


echo bc453rtl.dll   Borland C++ Runtime 4.53 DLL fÅr sosdb16.dll u.a.
compress s:\bc4\bin\bc453rtl.dll bc453rtl.dl_ > %TMP%\makedisk.log
if errorlevel 1 goto ERROR

echo ctl3v2.dll     3D-Controls
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\ctl3dv2.dl_
if errorlevel 1 goto ERROR



rem echo odbcinst.cpl   ODBC Konfiguration, Datei fÅr Systemsteuerung
rem %makedisk_copy% s:\ms\odbcsdk.200\drvsetup.kit\setup16\odbcinst.cp_
rem if errorlevel 1 goto ERROR

echo odbcadm.exe    ODBC Datenquellenverwalter
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\odbcadm.ex_
if errorlevel 1 goto ERROR

echo odbcinst.dll   ODBC Konfiguration
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\odbcinst.dl_
if errorlevel 1 goto ERROR

echo odbcinst.hlp   ODBC Hilfe fÅr Konfiguration
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\odbcinst.hl_
if errorlevel 1 goto ERROR


echo odbc.dll       ODBC Manager
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\odbc.dl_
if errorlevel 1 goto ERROR

echo odbccurs.dll   ODBC Cursor Library
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\odbccurs.dl_
if errorlevel 1 goto ERROR


echo cpn16ut.dll    ODBC 32 auf 16  universal thunk
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\cpn16ut.dl_
if errorlevel 1 goto ERROR

echo odbccp32.dll   ODBC 32 auf 16  thunking
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\odbccp32.dl_
if errorlevel 1 goto ERROR

echo odbc16ut.dll   ODBC 32 auf 16  setup generic thunking 32 to 16
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\odbc16ut.dl_
if errorlevel 1 goto ERROR

echo odbc32.dll     ODBC 32 auf 16  32bit
%makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\odbc32.dl_
if errorlevel 1 goto ERROR

rem echo ds16gt.dll
rem %makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\ds16gt.dl_
rem if errorlevel 1 goto ERROR

rem echo ds32gt.dll     ODBC 32bit setup generic thunking 16 to 32
rem %makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\ds32gt.dl_
rem if errorlevel 1 goto ERROR

rem echo odbccr32.dll   ODBC 32bit cursor library
rem %makedisk_copy% s:\ms\odbcsdk.200\redist16.ger\odbccr32.dl_
rem if errorlevel 1 goto ERROR


set makedisk_copy=

goto ENDE

:ERROR
echo Fehler aufgetreten:
more %TMP%\makedisk.log
goto :ENDE

:QUELL_LAUFWERK
echo Nicht auf dem Quell-Laufwerk aufrufen!
goto :ENDE

:USAGE
echo usage: makedisk FROMDIR
echo ˇ
echo Kopiert die Dateien fÅr die ODBC-Installationsdiskette ins aktuelle
echo Verzeichnis.
echo sosdb16.dll, sosdb16i.dll und sosodbcs.exe werden aus FROMDIR Åbernommen.
echo bc453rtl.dll wird aus s:\bc4\bin\bc453rtl.dll Åbernommen.
echo setup.lst, odbc.inf, odbc.ini, odbcinst.ini und soserror.txt werden
echo direkt aus k:\e\prod\sosodbc Åbernommen

:ENDE








