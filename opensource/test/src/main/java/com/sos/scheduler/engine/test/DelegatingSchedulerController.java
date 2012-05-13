package com.sos.scheduler.engine.test;

import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.main.SchedulerController;
import com.sos.scheduler.engine.main.SchedulerThreadController;

abstract class DelegatingSchedulerController implements SchedulerController {
    private final SchedulerThreadController delegate;

    protected DelegatingSchedulerController(String name) {
        delegate =  new SchedulerThreadController(name);
    }

    @Override public final void setSettings(Settings o) {
        delegate.setSettings(o);
    }

    @Override public final Settings getSettings() {
        return delegate.getSettings();
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

    @Override public final SchedulerEventBus getEventBus() {
        return delegate.getEventBus();
    }

    protected final SchedulerThreadController getDelegate() {
        return delegate;
    }
}
