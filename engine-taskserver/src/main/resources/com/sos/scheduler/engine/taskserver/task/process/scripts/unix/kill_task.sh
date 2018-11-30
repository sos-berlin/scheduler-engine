#! /bin/sh
#  ------------------------------------------------------------
#  Company: Software- und Organisations-Service GmbH
#  Purpose: "kill script" for JobScheduler Agent
#  ------------------------------------------------------------

set -e

#waiting time until a SIGKILL is sent after a SIGTERM
KILL_WITHIN=15

log() {
    date "+%Y-%m-%d %T,%3N %z [$1]  $2" 1>&2
}

running()
{
  if kill -0 $1 2>/dev/null
  then
    return 0
  fi
  return 1
}

PID=""
for arg in "$@"; do
    case "$arg" in
        -kill-agent-task-id=*)
            AGENT_TASK_ID=`echo "$arg" | sed 's/-kill-agent-task-id=//'`
            ;;
        -master-task-id=*)
            MASTER_TASK_ID=`echo "$arg" | sed 's/-master-task-id=//'`
             ;;
        -job=*)
            JOB_PATH=`echo "$arg" | sed 's/-job=//'`
            ;;
        -pid=*)
            PID=`echo "$arg" | sed 's/-pid=//'`
            ;;
    esac
done

if [ -z "$AGENT_TASK_ID$PID" ]; then
    log error "Option -kill-agent-task-id is not set"
    exit 2
fi
if [ ! -z "$JOB_PATH" ]; then
    log info "Task '$MASTER_TASK_ID' of Job '$JOB_PATH' with Agent task id '$AGENT_TASK_ID' is being killed"
else
    log info "Task with Agent task id '$AGENT_TASK_ID' is being killed"
fi

TASK_PID=`ps ww | grep " -agent-task-id[=]$AGENT_TASK_ID\\b" | awk '{ print $1 }'`
[ ! -z "$TASK_PID" ] || TASK_PID="$PID"
if [ -z "$TASK_PID" ]; then
    log info "Process with -agent-task-id=$AGENT_TASK_ID doesn't exist"
    exit 0
fi

log info "Killing task with PID $TASK_PID and its children"
descendants=

if [ "`uname`" = "SunOS" ]; then
    psTree="ps -ef -o pid,ppid"
else
    psTree="ps ax -o pid,ppid"
fi

stopTask() {
    if [ $KILL_WITHIN -gt 0 ]
    then
        log info "kill -TERM $TASK_PID  (the task process)"
        kill -15 $TASK_PID
        LOOP_COUNTER=0
        while [ $LOOP_COUNTER -lt $KILL_WITHIN ]
        do
            if running $TASK_PID
            then
                LOOP_COUNTER=`expr ${LOOP_COUNTER} + 1`
                sleep 1
            else
                break
            fi
        done
    fi
}

collectAndStopAllPids() {
    # First stop all processes to inhibit quickly forking parent from producing children between child killing and parent killing
    # $1: Parent PID
    # $2: Indentation
    log info "$2 kill -STOP $1"
    kill -STOP $1 || true
    for _child in `$psTree | egrep " $1\$" | awk '{ print $1 }'`; do
        descendants="$_child $descendants"
        collectAndStopAllPids "$_child" "| $2"
    done
}


stopTask
if running $TASK_PID
then
  collectAndStopAllPids "$TASK_PID"
fi

exitCode=0

if running $TASK_PID
then
  log info "kill -KILL $TASK_PID  (the task process)"
  kill -KILL $TASK_PID || exitCode=1
fi

for pid in $descendants; do
    log info "kill -KILL $pid"
    kill -KILL $pid 2>/dev/null || true
done

exit $exitCode
