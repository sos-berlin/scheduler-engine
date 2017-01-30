#! /usr/bin/env bash
set -e

dir="/tmp/jobscheduler-smoke-test"
engineMainDir=$(cd "$(dirname -- "$0")"; pwd)

if [ -e "$dir" ]; then
    echo DELETING TEMPORARY DIRECTORY $dir
    sleep 3
    rm -r "$dir"
fi
mkdir "$dir"
cd "$dir"
tar xzf "$engineMainDir"/target/engine-main-bin.tar.gz

unset SOS_INI
unset SCHEDULER_HOME
export SCHEDULER_DATA='$SCHEDULER_HOME/data'  # Not substituted
engine/bin/jobscheduler.sh -http-port= -cmd='<order job_chain="/test" id="TEST"/>' &
enginePid=$!
(
    i=0
    while [ $i -lt 30 ]; do
        i=$(($i + 1))
        sleep 1
    done
    kill -SIGKILL $enginePid
) &
sleepingKillPid=$!
wait $enginePid
kill $sleepingKillPid
wait $sleepingKillPid &>/dev/null || true

returnCode=0
for job in test-100 test-200 test-300; do
    if [ -f $job.touched ]; then
        echo Job $job has run
    else
        echo ERROR: JOB $job HAS NOT RUN
        returnCode=1
    fi
done

rm -rf "$dir"

if [ $returnCode -eq 0 ]; then
    echo Smoke test succeeded
fi
exit $returnCode
