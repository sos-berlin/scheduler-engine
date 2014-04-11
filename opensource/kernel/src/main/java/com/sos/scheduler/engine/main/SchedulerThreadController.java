package com.sos.scheduler.engine.main;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.common.sync.ThrowableMailbox;
import com.sos.scheduler.engine.common.time.Stopwatch;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.CppSettings;
import org.joda.time.Duration;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.util.concurrent.TimeUnit;

import static com.google.common.base.Preconditions.checkState;

/** Steuert den {@link SchedulerThread}. */
public class SchedulerThreadController implements SchedulerController {
    private static final Logger logger = LoggerFactory.getLogger(SchedulerThreadController.class);
    private static final Duration terminationTimeout = Duration.standardSeconds(5);

    private final String name;
    private final SchedulerEventBus eventBus = new SchedulerEventBus();
    private boolean isStarted = false;
    private final ThrowableMailbox<Throwable> throwableMailbox = new ThrowableMailbox<Throwable>();
    private final SchedulerThreadControllerBridge controllerBridge;
    private final SchedulerThread thread;

    public SchedulerThreadController(String name, CppSettings cppSettings) {
        this.name = name;
        controllerBridge = new SchedulerThreadControllerBridge(this, eventBus, cppSettings);
        thread = new SchedulerThread(controllerBridge);
    }

    public final void loadModule(File cppModuleFile) {
        SchedulerThread.loadModule(cppModuleFile);
    }

    @Override public final void startScheduler(Iterable<String> args) {
        checkIsNotStarted();
        controllerBridge.start();
        thread.startThread(ImmutableList.copyOf(args));
        isStarted = true;
    }

    @Override public final void close() {
        Stopwatch stopwatch = new Stopwatch();
        terminateScheduler();
        if (!tryJoinThread(terminationTimeout)) {
            logger.warn("Still waiting for JobScheduler termination ("+terminationTimeout+") ...");
            tryJoinThread(new Duration(Long.MAX_VALUE));
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
        controllerBridge.terminate();
    }

    @Override public final boolean tryWaitForTermination(Duration timeout) {
        checkIsStarted();
        boolean result = tryJoinThread(timeout);
        throwableMailbox.throwUncheckedIfSet();
        return result;
    }

    private boolean tryJoinThread(Duration timeout) {
        try {
            if (timeout.getMillis() == Long.MAX_VALUE)  thread.join();
            else TimeUnit.MILLISECONDS.timedJoin(thread, timeout.getMillis());
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

    public String getName() {
        return name;
    }
}
