#! /bin/bash
set -e

[ -z "$SCHEDULER_AGENT_HOME" ] && SCHEDULER_AGENT_HOME=$(cd "$(dirname "$0")/.."; pwd)

if [ "$OSTYPE" = "cygwin" ]; then
    jarDir=$(cygpath -w "$SCHEDULER_AGENT_HOME/jar")
else
    jarDir="$SCHEDULER_AGENT_HOME/jar"
fi

java -classpath "$jarDir/*" com.sos.scheduler.engine.agent.main.Main "$@"
