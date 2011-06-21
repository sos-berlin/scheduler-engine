#!/bin/bash
if [ $# != 1 ]
then
  echo "usage: prepare-setup.sh <version>"
  exit 99
fi
version=$1
mvn dependency:copy dependency:copy-dependencies antrun:run -Prelease -Dsetup.version=$version -Dsetup.platform=windows-x86
mvn dependency:copy dependency:copy-dependencies antrun:run -Prelease -Dsetup.version=$version -Dsetup.platform=linux-i386
