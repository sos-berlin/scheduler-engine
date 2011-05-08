rem @echo off

c:
cd \tmp
rem jz 15.8.94 s:\borlandc\bin\hc31 g:..\%1
hcp k:..\..\..\prod\%1\%1
del k:%1.hlp
del \~hc*
k:     
