package com.sos.scheduler.engine.main;

import com.sos.scheduler.engine.kernel.Scheduler;
import java.util.concurrent.atomic.AtomicReference;
import static com.sos.scheduler.engine.main.BridgeState.started;
import static com.sos.scheduler.engine.main.BridgeState.starting;
import static com.sos.scheduler.engine.main.BridgeState.subsystemClosed;

final class SchedulerStateBridge {
    private final AtomicReference<Scheduler> schedulerAtom = new AtomicReference<Scheduler>();
    private volatile BridgeState state = starting;

    synchronized void setStateStarted(Scheduler scheduler) {
        schedulerAtom.set(scheduler);
        state = started;
        notifyAll();
    }

    synchronized void setState(BridgeState s) {
        state = s;
        notifyAll();
    }

    synchronized void setStateClosed() {
        state = subsystemClosed;
        schedulerAtom.set(null);
        notifyAll();
    }

    synchronized Scheduler waitUntilSchedulerState(BridgeState awaitedState) throws InterruptedException {
        while (state.ordinal() < awaitedState.ordinal())
            wait();
        return scheduler();
    }

    Scheduler scheduler() {
        return schedulerAtom.get();
    }
}
