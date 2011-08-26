rem $Id: bin-to-sos.bat 11347 2005-03-24 10:14:54Z jz $
rem Kopiert bin\*.gz auf cvs.sos

rem gzip -fN bin\*.map
rem gzip -fN bin\sosfact.dll

del /q l:\tmp\sos-bin\*
xcopy bin\*.exe l:\tmp\sos-bin\
xcopy bin\*.dll l:\tmp\sos-bin\
xcopy bin\*.map l:\tmp\sos-bin\
@rem xcopy bin\*.jar l:\tmp\sos-bin\   Nur die Linux-Variante nehmen!
xcopy bin\sos.spooler.jar l:\tmp\sos-bin\
xcopy hostphp\hostphp.inc l:\tmp\sos-bin\
xcopy hostphp\hostware_file.inc l:\tmp\sos-bin\
xcopy hostphp\hostware_dynobj.inc l:\tmp\sos-bin\
xcopy spooler\spooler.dtd l:\tmp\sos-bin\
xcopy spooler\spooler.xml l:\tmp\sos-bin\
xcopy spooler\add_jobs.xml l:\tmp\sos-bin\

rsh loka rsync -azv --progress ~/tmp/sos-bin/* cvs.sos:/home/sos/jz

rem @for %%f in (bin\*.gz) do rcp -b %%f cvs.sos.%username%:/home/sos/jz/
rem rsh cvs.sos gzip -dfNv /home/sos/jz/*.gz


rem :A
rem rsync -azv --progress bin/hostole.dll bin/hostjava.dll bin/hostjava.jar bin/hostap32.dll bin/hostphp.dll bin/sosodbc.dll bin/hostcopy.exe bin/spooler.exe bin/sos.spooler.jar bin/sosfact.dll bin/sosfs.exe cvs.sos:/home/sos/jz/
