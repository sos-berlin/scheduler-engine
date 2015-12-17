#! /bin/bash -E

declare -a javaOptions
declare -a agentOptions
logDirectory=
httpPort=

for arg in "$@"; do
    case $arg in
        -rmx-port=*)
            port=${arg/-rmx-port=/}
            javaOptions=(
                "-Dcom.sun.management.jmxremote"
                "-Dcom.sun.management.jmxremote.ssl=false"
                "-Dcom.sun.management.jmxremote.authenticate=false"
                "-Dcom.sun.management.jmxremote.port=$port")
            shift
            ;;
        -intellij-port=*)
            port=${arg/-intellij-port=/}
            javaOptions=("-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=$port")
            shift
            ;;
        -http-port=*)
            httpPort="${arg#-http-port=}"
            agentOptions+=("$arg")
            shift
            ;;
        -log-directory=*)
            logDirectory="${arg#-log-directory=}"
            agentOptions+=("$arg")
            shift
            ;;
        *)
            agentOptions+=("$arg")
            shift
            ;;
    esac
done


bin=$(cd "$(dirname "$0")"; pwd)
if [ -z "$SCHEDULER_AGENT_HOME" ]; then :
    SCHEDULER_AGENT_HOME=$(cd "$bin/.."; pwd)
fi
. "$bin/set-context.sh"

if [ -n "$httpPort$logDirectory" ]; then :
    crashKillScript="$logDirectory/kill_tasks_after_crash_$httpPort.sh"
else
    crashKillScript=
fi

if [ -f "$SCHEDULER_AGENT_WORK/etc/logback.xml" ]; then :
    logbackConfig="file:$SCHEDULER_AGENT_WORK/etc/logback.xml"
else
    logbackConfig="com/sos/scheduler/engine/agent/main/logback.xml"
fi

logbackArg="-Dlogback.configurationFile=$logbackConfig"
agentOptions=("-job-java-options=$logbackArg" "${agentOptions[@]}")
executeAgent=("$java" "${javaOptions[@]}" -classpath "$jarDir/*" "$logbackArg" com.sos.scheduler.engine.agent.main.AgentMain "${agentOptions[@]}")
echo "${executeAgent[@]}"
if [ -n "$crashKillScript" ]; then :
    rm -f "$crashKillScript"
    set +E
    "${executeAgent[@]}"
    returnCode=$?
    if [ -s "$crashKillScript" ]; then :
        ps fux
        echo Executing crash kill script $crashKillScript:
        cat $crashKillScript
        (. "$crashKillScript" || true)
        ps fux
    else
       echo "No tasks running ($crashKillScript is empty)"
    fi
    set -E
    exit $returnCode
else
    exec "${executeAgent[@]}"
fi
