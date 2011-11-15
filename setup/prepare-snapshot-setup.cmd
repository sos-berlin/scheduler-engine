@echo off
@REM ---------------------------------------------------------------------------------------------------------------
@REM /brief Prepare the resources of the scheduler kernel for setup
@REM /details
@REM This script is for downloading and preparing the necessary resources of the scheduler kernel from archiva.
@REM
@REM in a windows environment you can use 'bash -c "./prepare-snapshot-setup.sh"' if cygwin is installed.
@REM ---------------------------------------------------------------------------------------------------------------
call mvn clean
call mvn install -Pprepare-snapshot -Dplatform=windows-x86
call mvn install -Pprepare-snapshot -Dplatform=linux-i386
