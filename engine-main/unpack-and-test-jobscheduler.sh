#! /usr/bin/env bash
set -e

dir="/tmp/jobscheduler-smoke-test"
engineMainDir=$(cd "$(dirname "$0")"; pwd)

if [ -e "$dir" ]; then
    echo DELETING TEMPORARY DIRECTORY $dir
    sleep 3
    rm -r "$dir"
fi
mkdir "$dir"
cd "$dir"
tar xzf "$engineMainDir"/target/engine-main-bin.tar.gz

export SCHEDULER_DATA="$dir"/data
mkdir "$SCHEDULER_DATA"
mkdir "$SCHEDULER_DATA"/logs
cp -r "$dir"/engine/config "$SCHEDULER_DATA"/
cp -r "$dir"/engine/test/live "$SCHEDULER_DATA"/config/

engine/bin/jobscheduler.sh -cmd='<order job_chain="/test" id="TEST"/>' &
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
