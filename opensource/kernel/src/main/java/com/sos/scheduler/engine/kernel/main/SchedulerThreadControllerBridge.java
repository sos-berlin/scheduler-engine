package com.sos.scheduler.engine.kernel.main;

import static com.sos.scheduler.engine.kernel.main.SchedulerState.active;
import static com.sos.scheduler.engine.kernel.main.SchedulerState.started;
import static com.sos.scheduler.engine.kernel.main.SchedulerState.terminated;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.main.event.TerminatedEvent;
import com.sos.scheduler.engine.kernel.schedulerevent.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.settings.Settings;

final class SchedulerThreadControllerBridge implements SchedulerControllerBridge, EventHandlerAnnotated {
    private final SchedulerThreadController schedulerThreadController;
    private final SchedulerEventBus eventBus;
    private final SchedulerStateBridge stateBridge = new SchedulerStateBridge();

    SchedulerThreadControllerBridge(SchedulerThreadController c, SchedulerEventBus eventBus) {
        this.schedulerThreadController = c;
        this.eventBus = eventBus;
    }

    void start() {
        eventBus.registerAnnotated(this);
    }

    void close() {
        eventBus.unregisterAnnotated(this);
    }

    @Override public Settings getSettings() {
        return schedulerThreadController.getSettings();
    }

    @Override public void onSchedulerStarted(Scheduler scheduler) {
        boolean terminate = stateBridge.setStateStarted(scheduler);
        if (terminate)  scheduler.terminate();
        eventBus.publish(new SchedulerReadyEvent());
    }

    @Override public void onSchedulerActivated() {
        stateBridge.setState(active);
    }

    @Override public void onSchedulerTerminated(int exitCode, @Nullable Throwable t) {
        if (t != null) schedulerThreadController.setThrowable(t);
        stateBridge.setState(terminated);
        eventBus.publish(new TerminatedEvent(exitCode, t));
    }

    @Override public SchedulerEventBus getEventBus() {
        return eventBus;
    }

    @HotEventHandler public void handleEvent(SchedulerCloseEvent e) {
        stateBridge.setStateClosed();
    }

    synchronized Scheduler waitWhileSchedulerIsStarting() throws InterruptedException {
        waitUntilSchedulerState(started);
        return stateBridge.scheduler();
    }

    synchronized void waitUntilSchedulerState(SchedulerState awaitedState) throws InterruptedException {
        stateBridge.waitUntilSchedulerState(awaitedState);
    }

    void terminate() {
        Scheduler scheduler = stateBridge.tryTerminateAndGetScheduler();
        if (scheduler != null) scheduler.terminate();
    }
}
