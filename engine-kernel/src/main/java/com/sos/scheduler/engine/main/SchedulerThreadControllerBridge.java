package com.sos.scheduler.engine.main;

import com.sos.scheduler.engine.data.scheduler.SchedulerCloseEvent;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.CppSettings;
import com.sos.scheduler.engine.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.main.event.TerminatedEvent;
import javax.annotation.Nullable;
import static com.sos.scheduler.engine.main.BridgeState.active;
import static com.sos.scheduler.engine.main.BridgeState.terminated;

final class SchedulerThreadControllerBridge implements SchedulerControllerBridge, EventHandlerAnnotated {
    private final SchedulerThreadController schedulerThreadController;
    private final SchedulerEventBus eventBus;
    private final CppSettings cppSettings;
    private final SchedulerStateBridge stateBridge = new SchedulerStateBridge();
    private volatile boolean terminateSchedulerWhenPossible = false;

    SchedulerThreadControllerBridge(SchedulerThreadController c, SchedulerEventBus eventBus, CppSettings cppSettings) {
        this.schedulerThreadController = c;
        this.eventBus = eventBus;
        this.cppSettings = cppSettings;
    }

    void start() {
        eventBus.registerAnnotated(this);
    }

    void close() {
        eventBus.unregisterAnnotated(this);
    }

    @Override public String getName() {
        return schedulerThreadController.getName();
    }

    @Override public CppSettings cppSettings() {
        return cppSettings;
    }

    @Override public void onSchedulerStarted(Scheduler scheduler) {
        stateBridge.setStateStarted(scheduler);
        eventBus.publish(new SchedulerReadyEvent());
        if (terminateSchedulerWhenPossible)  scheduler.terminate();
    }

    @Override public void onSchedulerActivated() {
        stateBridge.setState(active);
    }

    @Override public void onSchedulerTerminated(int exitCode, @Nullable Throwable t) {
        if (t != null) schedulerThreadController.setThrowable(t);
        stateBridge.setState(terminated);
        eventBus.publish(new TerminatedEvent(exitCode, t));
        eventBus.dispatchEvents();
    }

    @Override public SchedulerEventBus getEventBus() {
        return eventBus;
    }


    @Override public boolean isCppThreadRequired() {
        return false;
    }

    @HotEventHandler public void handleEvent(SchedulerCloseEvent e) {
        stateBridge.setStateClosed();
    }

    Scheduler waitUntilSchedulerState(BridgeState awaitedState) throws InterruptedException {
        return stateBridge.waitUntilSchedulerState(awaitedState);
    }

    void terminate() {
        Scheduler scheduler = stateBridge.scheduler();
        if (scheduler != null) scheduler.terminate();
        else terminateSchedulerWhenPossible = true;
    }
}
