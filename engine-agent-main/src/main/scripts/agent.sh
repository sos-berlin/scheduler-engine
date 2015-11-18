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

if [ -z "$SCHEDULER_AGENT_HOME" ]; then :
    SCHEDULER_AGENT_HOME=$(cd "$(dirname "$0")/.."; pwd)
fi
. "$SCHEDULER_AGENT_HOME/bin/set-context.sh"

if [ -f "$SCHEDULER_AGENT_WORK/etc/logback.xml" ]; then :
    logbackConfig="file:$SCHEDULER_AGENT_WORK/etc/logback.xml"
else
    logbackConfig="com/sos/scheduler/engine/agent/main/logback.xml"
fi

echo "$java" "${javaOptions[@]}" -classpath "$jarDir/*" -Dlogback.configurationFile="$logbackConfig" com.sos.scheduler.engine.agent.main.AgentMain "${agentOptions[@]}"
exec "$java" "${javaOptions[@]}" -classpath "$jarDir/*" -Dlogback.configurationFile="$logbackConfig" com.sos.scheduler.engine.agent.main.AgentMain "${agentOptions[@]}"
