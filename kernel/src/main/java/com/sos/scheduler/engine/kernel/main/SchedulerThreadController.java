package com.sos.scheduler.engine.kernel.main;

import static com.google.common.base.Preconditions.checkState;
import static com.google.common.base.Throwables.propagate;

import java.io.File;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.main.event.TerminatedEvent;
import com.sos.scheduler.engine.kernel.schedulerevent.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.ThrowableMailbox;

/** Steuert den {@link SchedulerThread}. */
public class SchedulerThreadController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(SchedulerThreadController.class);

    private final EventBus eventBus = new EventBus();
    private Settings settings = DefaultSettings.singleton;
    private boolean started = false;
    private final ThrowableMailbox<Throwable> throwableMailbox = new ThrowableMailbox<Throwable>();
    private final SchedulerThread thread;
    private final StateThreadBridge stateThreadBridge = new StateThreadBridge();

    public SchedulerThreadController() {
        thread = new SchedulerThread(new MyControllerBridge());
    }

    @Override public void setSettings(Settings o) {
        checkState(!started, "Scheduler has already been started");
        settings = o;
    }

    public final void loadModule(File cppModuleFile) {
        thread.loadModule(cppModuleFile);
    }

    @Override public final void startScheduler(String... args) {
        checkState(!started, "Scheduler has already been started");
        thread.startThread(args);
        started = true;
    }

    @Override public final Scheduler waitUntilSchedulerIsRunning() {
        checkState(started, "Scheduler has not been started");
        try {
            Scheduler result = stateThreadBridge.waitWhileSchedulerIsStarting();
            throwableMailbox.throwUncheckedIfSet();
            if (result == null) {
                throw new SchedulerException("Scheduler aborted before startup");
            }
            return result;
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }

    @Override public final void waitUntilSchedulerState(SchedulerState s) {
        try {
            stateThreadBridge.waitUntilSchedulerState(s);
            throwableMailbox.throwUncheckedIfSet();
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }

//    @Override public final SchedulerState getSchedulerState() {
//        return stateThreadBridge.getState();
//    }

    @Override public final void terminateAndWait() {
        tryTerminateScheduler();
        tryWaitForTermination(Time.eternal);
    }

    private void tryTerminateScheduler() {
        try {
            terminateScheduler();
        } catch (Exception x) {
            if (x.toString().contains("Z-JAVA-111"))    // TODO Z-JAVA-111 als eigene Java-Exception zurückgeben
                logger.warn(x);
            else
                throw propagate(x);
        }
    }

    @Override public final boolean tryWaitForTermination(Time timeout) {
        try {
            if (timeout == Time.eternal)  thread.join();
            else timeout.unit.timedJoin(thread, timeout.value);
            throwableMailbox.throwUncheckedIfSet();
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
        return !thread.isAlive();
    }

    @Override public final void terminateAfterException(Throwable x) {
        throwableMailbox.setIfFirst(x);
        terminateScheduler();
    }

    @Override public final void terminateScheduler() {
        stateThreadBridge.terminate();
    }

    @Override public final int exitCode() {
        return thread.exitCode();
    }

    @Override public EventBus getEventBus() {
        return eventBus;
    }

    private final class MyControllerBridge implements SchedulerControllerBridge, EventHandlerAnnotated {
        @Override public Settings getSettings() {
            return settings;
        }

        @Override public void onSchedulerStarted(Scheduler scheduler) {
            stateThreadBridge.onSchedulerStarted(scheduler);
            eventBus.publishImmediately(new SchedulerReadyEvent(SchedulerThreadController.this, scheduler));
        }

        @Override public void onSchedulerActivated() {
            stateThreadBridge.onSchedulerActivated();
            eventBus.registerAnnotated(this);
        }

        @Override public void onSchedulerTerminated(int exitCode, @Nullable Throwable t) {
            if (t != null) throwableMailbox.setIfFirst(t);
            stateThreadBridge.onSchedulerTerminated();
            eventBus.publishImmediately(new TerminatedEvent(exitCode, t));
        }

        @Override public EventBus getEventBus() {
            return eventBus;
        }

        @EventHandler public void handleEvent(Event e) {
            if (e instanceof SchedulerCloseEvent)
                stateThreadBridge.onSchedulerClosed();
        }
    }
}
