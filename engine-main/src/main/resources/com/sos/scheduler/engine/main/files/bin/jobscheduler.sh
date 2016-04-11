#! /bin/bash
set -e

# Simple JobScheduler starter

# Example usage:
#   cd /tmp
#   rm -rf engine
#   tar xfz .../engine/engine-main/target/engine-main-bin.tar.gz
#   export SCHEDULER_DATA=data (containing config/, config/live/ and log/)
#   export SCHEDULER_WWW=$HOME/all/joc/joc-html-cockpit/target/classes (JOC's browser files)
#   engine/bin/jobscheduler.sh -i

declare -a javaOptions
declare -a engineOptions
tcpPort=4444

for arg in "$@"; do
    case $arg in
        -http-port=*)
            tcpPort="${arg#-tcp-port=}"
            shift
            ;;
        *)
            engineOptions+=("$arg")
            shift
            ;;
    esac
done

if [ -z "$SCHEDULER_HOME" ]; then :
    bin=$(cd "$(dirname "$0")" && pwd)
    SCHEDULER_HOME=$(cd "$bin"/.. && pwd || kill $$)
else
    bin="$SCHEDULER_HOME"/bin
fi
. "$bin/set-context.sh"

configDirectory="$SCHEDULER_DATA"/config
if [ ! -d "$configDirectory" ]; then :
    echo "Missing directory $configDirectory"
    exit 1
fi

liveDirectory="$SCHEDULER_DATA"/config/live
if [ ! -d "$liveDirectory" ]; then :
    echo "Missing directory $liveDirectory"
    exit 1
fi

if [ -f "$configDirectory/logback.xml" ]; then :
    logbackConfig="file:$configDirectory/logback.xml"
else
    logbackConfig="com/sos/scheduler/engine/kernel/logback.xml"
fi

logbackArg="-Dlogback.configurationFile=$logbackConfig"
javaOptions=("$logbackArg" "${javaOptions[@]}")

export LD_LIBRARY_PATH="$bin:$LD_LIBRARY_PATH"
executeEngine=(
    "$bin/scheduler"
    -sos.ini="$configDirectory/sos.ini"
    -ini="$configDirectory/scheduler.ini"
    -config="$configDirectory/scheduler.xml"
    -java-options="${javaOptions[@]}"
    -java-classpath="$jarDir/*.jar"
    -job-java-options="$logbackArg"
    -job-java-classpath="$jarDir/*.jar"
    -configuration-directory="$liveDirectory"
    -tcp-port="$tcpPort"
    "${engineOptions[@]}")
echo "${executeEngine[@]}"
exec "${executeEngine[@]}"
