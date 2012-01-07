package com.sos.scheduler.engine.main;

import static com.google.common.base.Preconditions.checkState;
import static com.google.common.base.Throwables.propagate;

import java.io.File;

import com.sos.scheduler.engine.kernel.util.Stopwatch;
import org.apache.log4j.Logger;

import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.ThrowableMailbox;

/** Steuert den {@link SchedulerThread}. */
public class SchedulerThreadController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(SchedulerThreadController.class);
    private static final Time terminationTimeout = Time.of(5);

    private final SchedulerEventBus eventBus = new SchedulerEventBus();
    private final Settings settings = new Settings();
    private boolean isStarted = false;
    private final ThrowableMailbox<Throwable> throwableMailbox = new ThrowableMailbox<Throwable>();
    private final SchedulerThreadControllerBridge controllerBridge = new SchedulerThreadControllerBridge(this, eventBus);
    private final SchedulerThread thread = new SchedulerThread(controllerBridge);

    @Override public final void setSettings(Settings o) {
        checkIsNotStarted();
        settings.setAll(o);
    }

    public final void loadModule(File cppModuleFile) {
        thread.loadModule(cppModuleFile);
    }

    @Override public final void startScheduler(String... args) {
        checkIsNotStarted();
        controllerBridge.start();
        thread.startThread(args);
        isStarted = true;
    }

    @Override public final void close() {
        Stopwatch stopwatch = new Stopwatch();
        terminateScheduler();
        if (!tryJoinThread(terminationTimeout)) {
            logger.warn("Still waiting for JobScheduler termination ("+terminationTimeout+") ...");
            tryJoinThread(Time.eternal);
            logger.info("JobScheduler has been terminated after "+stopwatch);
        }
        controllerBridge.close();
        eventBus.dispatchEvents();  // Thread-sicher, weil der Scheduler-Thread beendet ist, also selbst kein dispatchEvents() mehr aufrufen kann.
        throwableMailbox.throwUncheckedIfSet();
    }

    public final Scheduler waitUntilSchedulerState(SchedulerState s) {
        try {
            checkIsStarted();
            Scheduler scheduler = controllerBridge.waitUntilSchedulerState(s);
            throwableMailbox.throwUncheckedIfSet();
            return scheduler;
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
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

    @Override public final boolean tryWaitForTermination(Time timeout) {
        checkIsStarted();
        boolean result = tryJoinThread(timeout);
        throwableMailbox.throwUncheckedIfSet();
        return result;
    }

    private boolean tryJoinThread(Time timeout) {
        try {
            if (timeout == Time.eternal)  thread.join();
            else timeout.unit.timedJoin(thread, timeout.value);
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
        return !thread.isAlive();
    }

    public void checkIsNotStarted() {
        checkState(!isStarted, "Scheduler has already been started");
    }

    private void checkIsStarted() {
        checkState(isStarted, "Scheduler has not been started");
    }

    public final boolean isStarted() {
        return isStarted;
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
