package com.sos.scheduler.engine.kernel.scheduler;

import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.main.SchedulerControllerBridge;
import com.sos.scheduler.engine.kernel.settings.Settings;

public class EmptySchedulerControllerBridge implements SchedulerControllerBridge {
    public static final EmptySchedulerControllerBridge singleton = new EmptySchedulerControllerBridge();

    private final Settings settings = new Settings();
    private final SchedulerEventBus eventBus = new SchedulerEventBus();

    private EmptySchedulerControllerBridge() {}

    @Override public final Settings getSettings() {
        return settings;
    }

    @Override public final void onSchedulerStarted(Scheduler scheduler) {}

    @Override public final void onSchedulerActivated() {}

    @Override public final void onSchedulerTerminated(int exitCode, Throwable t) {}

    @Override public final SchedulerEventBus getEventBus() {
        return eventBus;
    }

    @Override public final String getName() {
        return EmptySchedulerControllerBridge.class.getSimpleName();
    }
}
