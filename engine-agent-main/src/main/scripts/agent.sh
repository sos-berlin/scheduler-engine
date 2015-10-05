#! /bin/bash
set -e

if [ -z "$SCHEDULER_AGENT_HOME" ]; then :
    SCHEDULER_AGENT_HOME=$(cd "$(dirname "$0")/.."; pwd)
fi
. "$SCHEDULER_AGENT_HOME/bin/set-context.sh"

if [ -f "$SCHEDULER_AGENT_WORK/etc/logback.xml" ]; then :
    logbackConfig="file:$SCHEDULER_AGENT_WORK/etc/logback.xml"
else
    logbackConfig="com/sos/scheduler/engine/agent/main/logback.xml"
fi

exec "$java" -classpath "$jarDir/*" -Dlogback.configurationFile="$logbackConfig" com.sos.scheduler.engine.agent.main.AgentMain "$@"
