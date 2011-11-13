package com.sos.scheduler.engine.kernel.main;

import static com.sos.scheduler.engine.kernel.main.SchedulerState.closed;
import static com.sos.scheduler.engine.kernel.main.SchedulerState.started;
import static com.sos.scheduler.engine.kernel.main.SchedulerState.starting;

import java.util.concurrent.atomic.AtomicReference;

import com.sos.scheduler.engine.kernel.Scheduler;

final class SchedulerStateBridge {
    private final AtomicReference<Scheduler> schedulerAtom = new AtomicReference<Scheduler>();
    private volatile boolean terminateSchedulerWhenPossible = false;
    private volatile SchedulerState state = starting;

    synchronized boolean setStateStarted(Scheduler scheduler) {
        schedulerAtom.set(scheduler);
        state = started;
        notifyAll();
        return terminateSchedulerWhenPossible;
    }

    synchronized void setState(SchedulerState s) {
        state = s;
        notifyAll();
    }

    synchronized void setStateClosed() {
        state = closed;
        schedulerAtom.set(null);
        notifyAll();
    }

    synchronized void waitUntilSchedulerState(SchedulerState awaitedState) throws InterruptedException {
        while (state.ordinal() < awaitedState.ordinal())
            wait();
    }

    synchronized Scheduler tryTerminateAndGetScheduler() {
        Scheduler scheduler = schedulerAtom.get();
        if (scheduler == null) terminateSchedulerWhenPossible = true;
        return scheduler;
    }

    Scheduler scheduler() {
        return schedulerAtom.get();
    }
}
