#! /bin/bash
set -e

[ -z "$SCHEDULER_AGENT_HOME" ] && SCHEDULER_AGENT_HOME=$(cd "$(dirname "$0")/.."; pwd)
. "$SCHEDULER_AGENT_HOME/bin/set-context.sh"

"$java" -classpath "$jarDir/*" -Dlogback.configurationFile="com/sos/scheduler/engine/agent/client/main/logback.xml" com.sos.scheduler.engine.agent.client.main.AgentClientMain "$@"
