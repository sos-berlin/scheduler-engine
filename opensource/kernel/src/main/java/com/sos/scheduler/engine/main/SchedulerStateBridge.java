package com.sos.scheduler.engine.main;

import com.sos.scheduler.engine.kernel.Scheduler;

import java.util.concurrent.atomic.AtomicReference;

import static com.sos.scheduler.engine.main.SchedulerState.closed;
import static com.sos.scheduler.engine.main.SchedulerState.started;
import static com.sos.scheduler.engine.main.SchedulerState.starting;

final class SchedulerStateBridge {
    private final AtomicReference<Scheduler> schedulerAtom = new AtomicReference<Scheduler>();
    private volatile SchedulerState state = starting;

    synchronized void setStateStarted(Scheduler scheduler) {
        schedulerAtom.set(scheduler);
        state = started;
        notifyAll();
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

    synchronized Scheduler waitUntilSchedulerState(SchedulerState awaitedState) throws InterruptedException {
        while (state.ordinal() < awaitedState.ordinal())
            wait();
        return scheduler();
    }

    Scheduler scheduler() {
        return schedulerAtom.get();
    }
}
