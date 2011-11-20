package com.sos.scheduler.engine.test;

import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.main.SchedulerController;
import com.sos.scheduler.engine.main.SchedulerState;
import com.sos.scheduler.engine.main.SchedulerThreadController;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Time;

abstract class DelegatingSchedulerController implements SchedulerController {
    protected final SchedulerThreadController delegate = new SchedulerThreadController();

    @Override public final void setSettings(Settings o) {
        delegate.setSettings(o);
    }

    @Override public final void waitUntilSchedulerState(SchedulerState s) {
        delegate.waitUntilSchedulerState(s);
    }

    @Override public final void terminateScheduler() {
        delegate.terminateScheduler();
    }

    @Override public final void terminateAfterException(Throwable x) {
        delegate.terminateAfterException(x);
    }

    @Override public final boolean tryWaitForTermination(Time timeout) {
        return delegate.tryWaitForTermination(timeout);
    }

    @Override public final int exitCode() {
        return delegate.exitCode();
    }

    @Override public SchedulerEventBus getEventBus() {
        return delegate.getEventBus();
    }
}
