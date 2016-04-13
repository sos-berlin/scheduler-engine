#! /bin/bash
set -e

arguments="$*"
agentTaskId=""
for arg in "$@"; do
    case $arg in
        -kill-agent-task-id=*)
            agentTaskId="${arg#*=}"
            shift
            ;;
        *)
            echo "Unknown option: $arg"
            exit 101
        ;;
    esac
done
[ -n "$agentTaskId" ] || { echo Missing argument -kill-agent-task-id=; exit 102; }

ps=$(mktemp)
ps -ef >$ps
count=$(grep --count --fixed-strings -- " -agent-task-id=$agentTaskId" <$ps || kill $$)
rm $ps
[ $count -eq 1 ] || {
    echo "Exactly one process with -agent-task-id=$agentTaskId is expected, not: $count"
    if [ $count -eq 0 ]; then
        exit 100
    else
        exit $count
    fi
}
