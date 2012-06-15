#!/bin/bash
# ---------------------------------------------------------------------------------------------------------------
# /brief Prepare the resources of the scheduler kernel for setup
# /details
# This script is for downloading and preparing the necessary resources of the scheduler kernel from archiva.
# The version conforms always to the given version of the setup Artifakt.
#
# in a windows environment you can use 'bash -c "./prepare-setup.sh"' if cygwin is installed.
# ---------------------------------------------------------------------------------------------------------------
mvn clean
mvn install -U -Pprepare-platform -Dengine.platform=windows-x86
mvn install -U -Pprepare-platform -Dengine.platform=linux-x86

