package com.sos.scheduler.engine.kernel.main;

import static com.sos.scheduler.engine.kernel.main.SchedulerState.active;
import static com.sos.scheduler.engine.kernel.main.SchedulerState.closed;
import static com.sos.scheduler.engine.kernel.main.SchedulerState.started;
import static com.sos.scheduler.engine.kernel.main.SchedulerState.starting;
import static com.sos.scheduler.engine.kernel.main.SchedulerState.terminated;

import java.util.concurrent.atomic.AtomicReference;

import com.sos.scheduler.engine.kernel.Scheduler;

/** Die Event-Handler onX() nehmen Status-Änderungen und das {@link Scheduler}-Objekt vom {@link SchedulerThread} entgegen
 * und stellen sie dem aufrufenden Threadn ({@link SchedulerController}) zur Verfügung. */
final class StateThreadBridge {
    private final AtomicReference<Scheduler> schedulerAtom = new AtomicReference<Scheduler>();
    private volatile boolean terminateSchedulerWhenPossible = false;
    private volatile SchedulerState state = starting;

    void onSchedulerStarted(Scheduler scheduler) {
        boolean callTerminate;
        synchronized (this) {
            schedulerAtom.set(scheduler);
            state = started;
            notify();
            callTerminate = terminateSchedulerWhenPossible;
        }
        if (callTerminate)  scheduler.terminate();
    }

    synchronized void onSchedulerActivated() {
        state = active;
        notify();
    }

    synchronized void onSchedulerClosed() {
        state = closed;
        schedulerAtom.set(null);
        notify();
    }

    synchronized void onSchedulerTerminated() {
        state = terminated;
        notify();
    }

    synchronized Scheduler waitWhileSchedulerIsStarting() throws InterruptedException {
        waitUntilSchedulerState(started);
        return schedulerAtom.get();
    }

    synchronized void waitUntilSchedulerState(SchedulerState awaitedState) throws InterruptedException {
        while (state.ordinal() < awaitedState.ordinal())
            wait();
    }

    void terminate() {
        Scheduler scheduler;
        synchronized (this) {
            scheduler = schedulerAtom.get();
            if (scheduler == null) terminateSchedulerWhenPossible = true;
        }
        if (scheduler != null) scheduler.terminate();
    }

    public final SchedulerState getState() {
        return state;
    }
}
