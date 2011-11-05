package com.sos.scheduler.engine.kernel.main;

import static com.google.common.base.Preconditions.checkState;
import static com.google.common.base.Throwables.propagate;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.main.event.TerminatedEvent;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.schedulerevent.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.ThrowableMailbox;

/** Steuert den {@link SchedulerThread}. */
public class SchedulerThreadController implements SchedulerController {
    //private static final Time terminationTimeout = Time.of(10);
    private static final Logger logger = Logger.getLogger(SchedulerThreadController.class);

    private final Settings settings;
    private boolean started = false;
    private final ThrowableMailbox<Throwable> throwableMailbox = new ThrowableMailbox<Throwable>();
    private final SchedulerThread thread;
    private final StateThreadBridge stateThreadBridge = new StateThreadBridge();
    private final List<EventSubscriber> eventSubscribers = new ArrayList<EventSubscriber>();

    public SchedulerThreadController() {
        this(DefaultSettings.singleton);
    }

    public SchedulerThreadController(Settings settings) {
        this.settings = settings;
        thread = new SchedulerThread(new MyControllerBridge());
    }

    @Override public final void subscribeEvents(EventSubscriber s) {
        eventSubscribers.add(s);
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

    @Override public final void terminateAndWait() {
        tryTerminateScheduler();
        waitForTermination(Time.eternal);
    }

    @Override public final void waitForTermination(Time timeout) {
        try {
            if (timeout == Time.eternal)  thread.join();
            else timeout.unit.timedJoin(thread, timeout.value);
            throwableMailbox.throwUncheckedIfSet();
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }

    @Override public final void terminateAfterException(Throwable x) {
        throwableMailbox.setIfFirst(x);
        terminateScheduler();
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

    @Override public final void terminateScheduler() {
        stateThreadBridge.terminate();
    }

    @Override public final int exitCode() {
        return thread.exitCode();
    }

    private final class MyControllerBridge implements SchedulerControllerBridge {
        private final MyEventSubscriber myEventSubscriber = new MyEventSubscriber();
        private Scheduler scheduler = null;

        @Override public Settings getSettings() {
            return settings;
        }

        @Override public void onSchedulerStarted(Scheduler s) {
            this.scheduler = s;
            stateThreadBridge.onSchedulerStarted(scheduler);
            reportStrictlyEvent(new SchedulerReadyEvent(SchedulerThreadController.this));
        }

        @Override public void onSchedulerActivated() {
            stateThreadBridge.onSchedulerActivated();
            scheduler.getEventSubsystem().subscribe(myEventSubscriber);
        }

        @Override public void onSchedulerTerminated(int exitCode, @Nullable Throwable t) {
            if (t != null) throwableMailbox.setIfFirst(t);
            stateThreadBridge.onSchedulerTerminated();
            reportStrictlyEvent(new TerminatedEvent(exitCode, t));
        }

        class MyEventSubscriber implements EventSubscriber {
            @Override public final void onEvent(Event e) throws Exception {
                if (e instanceof SchedulerCloseEvent)
                    stateThreadBridge.onSchedulerClosed();
                reportEvent(e);
            }
        }
    }

    final void reportStrictlyEvent(Event e) {
        try { 
            reportEvent(e);
        }
        catch (Throwable x) {
            throwableMailbox.setIfFirst(x);
        }
    }

    private void reportEvent(Event e) throws Exception {
        for (EventSubscriber s: eventSubscribers)
            s.onEvent(e);
    }
}
