package com.sos.scheduler.engine.main;

import com.google.inject.Injector;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.scheduler.SchedulerClosed$;
import com.sos.scheduler.engine.data.scheduler.SchedulerTerminatedEvent;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.CppSettings;
import javax.annotation.Nullable;
import scala.Option;
import static com.sos.scheduler.engine.main.BridgeState.active;
import static com.sos.scheduler.engine.main.BridgeState.terminated;
import static java.util.Objects.requireNonNull;

final class SchedulerThreadControllerBridge implements SchedulerControllerBridge, EventHandlerAnnotated {
    private final SchedulerThreadController schedulerThreadController;
    private final SchedulerEventBus eventBus;
    private final CppSettings cppSettings;
    private final SchedulerStateBridge stateBridge = new SchedulerStateBridge();
    private volatile boolean terminateSchedulerWhenPossible = false;
    private Injector injector = null;

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
        if (terminateSchedulerWhenPossible)  scheduler.terminate();
    }

    @Override public void onSchedulerActivated() {
        stateBridge.setState(active);
    }

    @Override public void onSchedulerTerminated(int exitCode, @Nullable Throwable t) {
        if (t != null) schedulerThreadController.setThrowable(t);
        stateBridge.setState(terminated);
        eventBus.publish(KeyedEvent.of(new SchedulerTerminatedEvent(exitCode, Option.apply(t))));
        eventBus.dispatchEvents();
    }

    @Override public SchedulerEventBus getEventBus() {
        return eventBus;
    }

    public Injector injector() {
        if (injector == null) throw new IllegalStateException("Injector has not yet been created");
        return injector;
    }

    @Override public void setInjector(Injector injector) {
        this.injector = injector;
    }

    @HotEventHandler public void handleEvent(KeyedEvent<SchedulerClosed$> e) {
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
