REM Beschreibung:
REM Batch zum Erzeugen der Factory-Komponenten Manager/Dialog/Spooler/FactoryOLE
REM $Id: factmake.bat 11347 2005-03-24 10:14:54Z jz $

s:\u\pushdir

k:
cd \e\prod\factory

start /w k:\e\vb6.lnk /make manager\factman.vbp
start /w k:\e\vb6.lnk /make dialog\dialog.vbp
start /w k:\e\vb6.lnk /make service\service.vbp

cd sosfact
call .\copy_cls
cd ..
start /w k:\e\vb6.lnk /make sosfact\sosfact.vbp

dir q:\bin\fact*.exe
dir q:\bin\sosfact.dll

s:\u\popdir
