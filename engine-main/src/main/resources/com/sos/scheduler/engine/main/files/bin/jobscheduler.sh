#!/usr/bin/env bash
set -e

# Simple JobScheduler starter

# Example usage:
#   cd /tmp
#   rm -rf engine
#   tar xfz .../engine/engine-main/target/engine-main-bin.tar.gz
#   export SCHEDULER_DATA=data (containing config/, config/live/ and log/)
#   export SCHEDULER_WWW=$HOME/all/joc/joc-html-cockpit/target/classes (JOC's browser files)
#   engine/bin/jobscheduler.sh -i

unset SOS_INI

javaOptions=()
engineOptions=()
httpPort=4444

for arg in "$@"; do
  case "$arg" in
    -http-port=*)
      httpPort="${arg#*=}"
      shift
      ;;
    *)
      engineOptions+=("$arg")
      shift
      ;;
  esac
done

if [ -n "$httpPort" ]; then
  engineOptions+=("-http-port=$httpPort")
fi

declare jarDir
. "$(cd "$(dirname -- "$0")" && pwd || kill $$)/set-context.sh"
declare pathSeparator
export SCHEDULER_HOME SCHEDULER_DATA

config="$SCHEDULER_DATA"/config
if [ ! -d "$config" ]; then :
  echo "Missing directory $config"
  exit 1
fi

live="$SCHEDULER_DATA"/config/live
if [ ! -d "$live" ]; then :
  echo "Missing directory $live"
  exit 1
fi

logs="$SCHEDULER_DATA/logs"

export SCHEDULER_LOGS="$logs"  # Usable in log4j2.xml
unset log4jArg
if [ -f "$config/log4j2.xml" ]; then :
  log4jArg="-Dlog4j.configurationFile=$config/log4j2.xml"
  javaOptions+=("-Dlog4j.configurationFile=$config/log4j2.xml")
fi

javaOptions=("$log4jArg" "${javaOptions[@]}")

export LD_LIBRARY_PATH="$SCHEDULER_HOME/bin:$LD_LIBRARY_PATH"
executeEngine=(
  "$SCHEDULER_HOME/bin/scheduler"
  -sos.ini="$config/sos.ini"
  -ini="$config/scheduler.ini"
  -config="$config/scheduler.xml"
  -java-options="${javaOptions[@]}"
  -java-classpath="$jarDir/*.jar$pathSeparator$jarDir/extra/*.jar"
  -job-java-options="$log4jArg"
  -job-java-classpath="$jarDir/*.jar"
  -configuration-directory="$live"
  "${engineOptions[@]}")
echo "${executeEngine[@]}"
exec "${executeEngine[@]}"
