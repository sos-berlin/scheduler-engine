package com.sos.scheduler.engine.kernel.scheduler;

import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.CppSettings;
import com.sos.scheduler.engine.main.SchedulerControllerBridge;

public class EmptySchedulerControllerBridge implements SchedulerControllerBridge {
    public static final EmptySchedulerControllerBridge singleton = new EmptySchedulerControllerBridge();

    private final CppSettings cppSettings = CppSettings.Empty();
    private final SchedulerEventBus eventBus = new SchedulerEventBus();

    private EmptySchedulerControllerBridge() {}

    @Override public final CppSettings cppSettings() {
        return cppSettings;
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
