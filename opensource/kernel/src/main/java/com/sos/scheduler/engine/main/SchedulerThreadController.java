package com.sos.scheduler.engine.main;

import static com.google.common.base.Preconditions.checkState;
import static com.google.common.base.Throwables.propagate;

import java.io.File;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.ThrowableMailbox;

/** Steuert den {@link SchedulerThread}. */
public class SchedulerThreadController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(SchedulerThreadController.class);

    private final SchedulerEventBus eventBus = new SchedulerEventBus();
    private Settings settings = DefaultSettings.singleton;
    private boolean isStarted = false;
    private final ThrowableMailbox<Throwable> throwableMailbox = new ThrowableMailbox<Throwable>();
    private final SchedulerThreadControllerBridge controllerBridge = new SchedulerThreadControllerBridge(this, eventBus);
    private final SchedulerThread thread = new SchedulerThread(controllerBridge);

    @Override public final void setSettings(Settings o) {
        checkState(!isStarted, "Scheduler has already been started");
        settings = o;
    }

    public final void loadModule(File cppModuleFile) {
        thread.loadModule(cppModuleFile);
    }

    @Override public final void startScheduler(String... args) {
        checkState(!isStarted, "Scheduler has already been started");
        controllerBridge.start();
        thread.startThread(args);
        isStarted = true;
    }

    @Override public final void close() {
        terminateScheduler();
        tryWaitForTermination(Time.eternal);
        assert !thread.isAlive();
        controllerBridge.close();
        eventBus.dispatchEvents();  // Thread-sicher, weil der Scheduler-Thread beendet ist, also selbst kein dispatchEvents() mehr aufrufen kann.
    }

    public final Scheduler waitUntilSchedulerState(SchedulerState s) {
        try {
            checkState(isStarted, "Scheduler has not been started");
            Scheduler scheduler = controllerBridge.waitUntilSchedulerState(s);
            throwableMailbox.throwUncheckedIfSet();
            return scheduler;
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }

//    @Override public final SchedulerState getSchedulerState() {
//        return stateThreadBridge.getState();
//    }

    @Override public final boolean tryWaitForTermination(Time timeout) {
        try {
            if (timeout == Time.eternal)  thread.join();
            else timeout.unit.timedJoin(thread, timeout.value);
            throwableMailbox.throwUncheckedIfSet();
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
        return !thread.isAlive();
    }

    @Override public final void terminateAfterException(Throwable t) {
        throwableMailbox.setIfFirst(t);
        terminateScheduler();
    }

    final void setThrowable(Throwable t) {
        throwableMailbox.setIfFirst(t);
    }

    @Override public final void terminateScheduler() {
        try {
            controllerBridge.terminate();
        } catch (Exception x) {
            if (x.toString().contains("Z-JAVA-111"))    // TODO Z-JAVA-111 als eigene Java-Exception zurückgeben
                logger.debug(x);
            else
                throw propagate(x);
        }
    }

    @Override public final int exitCode() {
        return thread.exitCode();
    }

    @Override public final SchedulerEventBus getEventBus() {
        return eventBus;
    }

    public final Settings getSettings() {
        return settings;
    }
}
