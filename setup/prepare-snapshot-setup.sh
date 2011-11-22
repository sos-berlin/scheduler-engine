#!/bin/bash
# ---------------------------------------------------------------------------------------------------------------
# /brief Prepare the resources of the scheduler kernel for setup
# /details
# This script is for downloading and preparing the necessary resources of the scheduler kernel from archiva.
#
# in a windows environment you can use 'bash -c "./prepare-snapshot-setup.sh"' if cygwin is installed.
# ---------------------------------------------------------------------------------------------------------------
mvn clean
mvn install -Pprepare-snapshot -Dengine.platform=windows-x86
mvn install -Pprepare-snapshot -Dengine.platform=linux-i386
