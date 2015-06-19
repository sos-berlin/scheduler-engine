#! /bin/bash
set -e

if [ -z "$SCHEDULER_AGENT_HOME" ]; then :
    SCHEDULER_AGENT_HOME=$(cd "$(dirname "$0")/.."; pwd)
fi
. "$SCHEDULER_AGENT_HOME/bin/set-context.sh"

"$java" -classpath "$jarDir/*" com.sos.scheduler.engine.agent.main.AgentMain "$@"
