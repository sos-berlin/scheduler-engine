REM aus prod ausführen
REM -zschimmer Spezialfunktionen aktivieren
REM -log-dir Protokollausgabe in Verzeichnis
REM -e zusätzlich zum logdir: Konsolenausgabe 
REM group=<gruppenname aus release.js> Job-Parameter (nur bei -zschimmer)
mkdir make_internal\live
mkdir Archive\scheduler
copy ..\..\jar\target\com.sos.scheduler.engine.jar make_internal\com.sos.scheduler.engine.jar
copy ..\..\..\jars\log4j-1.2.16.jar  make_internal\log4j-1.2.16.jar
bind\scheduler.exe -zschimmer -log-dir=*stderr -e make_internal\release.scheduler group=%1