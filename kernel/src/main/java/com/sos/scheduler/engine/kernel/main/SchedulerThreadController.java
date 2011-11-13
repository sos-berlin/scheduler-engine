package com.sos.scheduler.engine.kernel.main;

import static com.google.common.base.Preconditions.checkState;
import static com.google.common.base.Throwables.propagate;

import java.io.File;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.ThrowableMailbox;

/** Steuert den {@link SchedulerThread}. */
public class SchedulerThreadController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(SchedulerThreadController.class);

    private final EventBus eventBus = new EventBus();
    private Settings settings = DefaultSettings.singleton;
    private boolean isStarted = false;
    private final ThrowableMailbox<Throwable> throwableMailbox = new ThrowableMailbox<Throwable>();
    private final SchedulerThreadControllerBridge controllerBridge = new SchedulerThreadControllerBridge(this, eventBus);
    private final SchedulerThread thread = new SchedulerThread(controllerBridge);

    @Override public void setSettings(Settings o) {
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
        controllerBridge.close();
    }

    @Override public final Scheduler waitUntilSchedulerIsRunning() {
        checkState(isStarted, "Scheduler has not been started");
        try {
            Scheduler result = controllerBridge.waitWhileSchedulerIsStarting();
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
            controllerBridge.waitUntilSchedulerState(s);
            throwableMailbox.throwUncheckedIfSet();
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
                logger.warn(x);
            else
                throw propagate(x);
        }
    }

    @Override public final int exitCode() {
        return thread.exitCode();
    }

    @Override public EventBus getEventBus() {
        return eventBus;
    }

    public Settings getSettings() {
        return settings;
    }
}
