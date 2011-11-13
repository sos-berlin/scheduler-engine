package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.kernel.main.SchedulerControllerBridge;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.settings.Settings;

public class EmptySchedulerControllerBridge implements SchedulerControllerBridge {
    public static final EmptySchedulerControllerBridge singleton = new EmptySchedulerControllerBridge();

    private final EventBus eventBus = new EventBus();

    private EmptySchedulerControllerBridge() {}

    @Override public Settings getSettings() {
        return DefaultSettings.singleton;
    }

    @Override public void onSchedulerStarted(Scheduler scheduler) {}

    @Override public void onSchedulerActivated() {}

    @Override public void onSchedulerTerminated(int exitCode, Throwable t) {}

    @Override public EventBus getEventBus() {
        return eventBus;
    }
}
