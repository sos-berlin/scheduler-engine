#!/usr/bin/env bash
set -e

arguments="$*"
agentTaskId=""
for arg in "$@"; do
    case $arg in
        -kill-agent-task-id=*)
            agentTaskId="${arg#*=}"
            shift
            ;;
        -pid=*)
            pid="${arg#*=}"
            shift
            ;;
        *)
            echo "Unknown option: $arg"
            exit 101
        ;;
    esac
done
[ -n "$agentTaskId" ] || { echo Missing argument -kill-agent-task-id=; exit 102; }
