package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.main.SchedulerControllerBridge;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.settings.Settings;

public class EmptySchedulerControllerBridge implements SchedulerControllerBridge {
    public static final EmptySchedulerControllerBridge singleton = new EmptySchedulerControllerBridge();

    private final SchedulerEventBus eventBus = new SchedulerEventBus();

    private EmptySchedulerControllerBridge() {}

    @Override public Settings getSettings() {
        return DefaultSettings.singleton;
    }

    @Override public void onSchedulerStarted(Scheduler scheduler) {}

    @Override public void onSchedulerActivated() {}

    @Override public void onSchedulerTerminated(int exitCode, Throwable t) {}

    @Override public SchedulerEventBus getEventBus() {
        return eventBus;
    }
}
