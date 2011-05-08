@echo off
set file=%1

echo on
c:\cygwin\bin\perl ass-to-cobol.pl <%file%.ass >%file%.cob
 