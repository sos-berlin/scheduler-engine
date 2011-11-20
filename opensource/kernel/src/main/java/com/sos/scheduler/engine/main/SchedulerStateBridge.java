package com.sos.scheduler.engine.main;

import static com.sos.scheduler.engine.main.SchedulerState.closed;
import static com.sos.scheduler.engine.main.SchedulerState.started;
import static com.sos.scheduler.engine.main.SchedulerState.starting;

import java.util.concurrent.atomic.AtomicReference;

import com.sos.scheduler.engine.kernel.Scheduler;

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

    synchronized void waitUntilSchedulerState(SchedulerState awaitedState) throws InterruptedException {
        while (state.ordinal() < awaitedState.ordinal())
            wait();
    }

    Scheduler scheduler() {
        return schedulerAtom.get();
    }
}
