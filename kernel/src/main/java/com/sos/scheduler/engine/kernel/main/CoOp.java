package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.kernel.Scheduler;
import java.util.concurrent.atomic.AtomicReference;


class CoOp {
    private final AtomicReference<Scheduler> schedulerAtom = new AtomicReference<Scheduler>();
    private final Object schedulerAtomLock = new Object();
    private volatile boolean terminateSchedulerWhenPossible = false;
    private volatile SchedulerState state = SchedulerState.starting;


    void onSchedulerStarted(Scheduler scheduler) {
        boolean callTerminate = false;
        synchronized (schedulerAtomLock) {
            schedulerAtom.set(scheduler);
            state = state.running;
            schedulerAtomLock.notify();
            callTerminate = terminateSchedulerWhenPossible;
        }
        if (callTerminate)  scheduler.terminate();
    }


    void onSchedulerClosed() {
        synchronized (schedulerAtomLock) {
            state = SchedulerState.closed;
            schedulerAtom.set(null);
            schedulerAtomLock.notify();
        }
    }


    void onTerminated() {
        synchronized (schedulerAtomLock) {
            state = state.threadTerminated;
            schedulerAtomLock.notify();
        }
    }


    Scheduler waitWhileSchedulerIsStarting() throws InterruptedException {
        Scheduler result = null;
        synchronized (schedulerAtomLock) {
            while (state == SchedulerState.starting)
                schedulerAtomLock.wait();
            result = schedulerAtom.get();
        }
        return result;
    }


    void terminate() {
        Scheduler scheduler = null;
        synchronized (schedulerAtomLock) {
            scheduler = schedulerAtom.get();
            if (scheduler == null)  terminateSchedulerWhenPossible = true;
        }
        if (scheduler != null)  scheduler.terminate();
    }


    Scheduler getScheduler() {
        return schedulerAtom.get();
    }


    SchedulerState getState() {
        return state;
    }
}
