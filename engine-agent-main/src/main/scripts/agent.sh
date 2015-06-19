#! /bin/bash
set -e

[ -z "$SCHEDULER_AGENT_HOME" ] && SCHEDULER_AGENT_HOME=$(cd "$(dirname "$0")/.."; pwd)
. "$SCHEDULER_AGENT_HOME/bin/set-context.sh"

"$java" -classpath "$jarDir/*" com.sos.scheduler.engine.agent.main.AgentMain "$@"
