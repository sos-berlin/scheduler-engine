#!/bin/bash
# ---------------------------------------------------------------------------------------------------------------
# /brief Prepare the resources of the scheduler kernel for setup
# /details
# This script is for downloading and preparing the necessary resources of the scheduler kernel from archiva.
#
# in a windows environment you can use 'bash -c "./prepare-setup.sh <version>"' if cygwin is installed.
# ---------------------------------------------------------------------------------------------------------------
if [ $# != 1 ]
then
  echo "usage: prepare-setup.sh <version>"
  exit 99
fi

version=$1
mvn clean
mvn dependency:copy dependency:copy-dependencies antrun:run -Pprepare -Dsetup.version=$version -Dsetup.platform=windows-x86
mvn dependency:copy dependency:copy-dependencies antrun:run -Pprepare -Dsetup.version=$version -Dsetup.platform=linux-i386
