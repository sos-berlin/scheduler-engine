package com.sos.scheduler.engine.kernel.main;

import static com.sos.scheduler.engine.kernel.main.SchedulerState.*;

import com.sos.scheduler.engine.kernel.Scheduler;
import java.util.concurrent.atomic.AtomicReference;

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
        while (state == starting)
            wait();
        return schedulerAtom.get();
    }

    void terminate() {
        Scheduler scheduler;
        synchronized (this) {
            scheduler = schedulerAtom.get();
            if (scheduler == null) terminateSchedulerWhenPossible = true;
        }
        if (scheduler != null) scheduler.terminate();
    }
}
