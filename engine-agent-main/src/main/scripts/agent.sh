#! /bin/bash
set -e

declare -a javaOptions
declare -a agentOptions

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

if [ -f "$SCHEDULER_AGENT_WORK/etc/logback.xml" ]; then :
    logbackConfig="file:$SCHEDULER_AGENT_WORK/etc/logback.xml"
else
    logbackConfig="com/sos/scheduler/engine/agent/main/logback.xml"
fi

logbackArg="-Dlogback.configurationFile=$logbackConfig"
agentOptions=("-job-java-options=$logbackArg" "${agentOptions[@]}")
echo "$java" "${javaOptions[@]}" -classpath "$jarDir/*" $logbackArg com.sos.scheduler.engine.agent.main.AgentMain "${agentOptions[@]}"
exec "$java" "${javaOptions[@]}" -classpath "$jarDir/*" $logbackArg com.sos.scheduler.engine.agent.main.AgentMain "${agentOptions[@]}"
