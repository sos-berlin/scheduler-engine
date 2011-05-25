@REM --------------------------------------------------------------------------------------------
@REM pre-release Version veröffentlichen
@REM --------------------------------------------------------------------------------------------

set source_dir=..
set target_dir=R:\nobackup\sos\scheduler_snapshot\windows
set jar_dir=%SCHEDULER_DEVEL%\engine\jar\target

@rem echo copy %source_dir%\bin\scheduler.exe to %target_dir%\scheduler.exe
copy %source_dir%\bin\scheduler.exe %target_dir%\scheduler.exe

@rem echo copy %source_dir%\bind\scheduler.dll to %target_dir%\scheduler.dll
copy %source_dir%\bind\scheduler.dll %target_dir%\scheduler.dll

@rem echo copy %source_dir%\bind\scheduler.exe to %target_dir%\scheduler_dll.exe
copy %source_dir%\bind\scheduler.exe %target_dir%\scheduler_dll.exe

@rem echo copy %jar_dir%\com.sos.scheduler.engine-complete.jar to %target_dir%\sos.spooler.jar
copy %jar_dir%\com.sos.scheduler.engine-complete.jar %target_dir%\sos.spooler.jar

rem echo copy %source_dir%\scheduler\scheduler.xsd to %target_dir%\scheduler.xsd
copy %source_dir%\scheduler\scheduler.xsd %target_dir%\scheduler.xsd

@REM Doku zunächst nicht.
@REM copy %source_dir%\doc ...
@REM copy %source_dir%\doc\javadoc ...
