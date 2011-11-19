package com.sos.scheduler.engine.test;

import static com.google.common.base.Throwables.propagate;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;
import static com.sos.scheduler.engine.test.binary.TestCppBinaries.cppBinaries;

import java.util.Arrays;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventHandlerFailedEvent;
import com.sos.scheduler.engine.eventbus.EventSubscriberAdaptingEventSubscription;
import com.sos.scheduler.engine.eventbus.EventSubscription;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.CppBinary;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.kernel.util.Time;

public class TestSchedulerController extends DelegatingSchedulerController implements EventHandlerAnnotated {
    private static final Logger logger = Logger.getLogger(TestSchedulerController.class);
    public static final Time shortTimeout = Time.of(10);

    private final Environment environment;
    private boolean terminateOnError = true;
    private Scheduler scheduler = null;

    public TestSchedulerController(ResourcePath resourcePath) {
        environment = new Environment(resourcePath);
    }

    /** Bricht den Test mit Fehler ab, wenn ein {@link com.sos.scheduler.engine.kernel.log.ErrorLogEvent} ausgelöst worden ist. */
    public final void setTerminateOnError(boolean o) {
        terminateOnError = o;
    }

    @EventHandler @HotEventHandler  // Beide, weil das Event wird nur innerhalb von Hot- oder ColdEventBus veröffentlicht wird.
    public final void handleEvent(EventHandlerFailedEvent e) {
        if (terminateOnError)
            terminateAfterException(e.getThrowable());
    }

    public final void subscribeEvents(EventSubscription s) {
        delegate.getEventBus().registerHot(s);
    }

    @Deprecated
    public final void subscribeEvents(EventSubscriber s) {
        delegate.getEventBus().registerHot(new EventSubscriberAdaptingEventSubscription(s));
    }

    /** @param timeout Wenn ab Bereitschaft des Schedulers mehr Zeit vergeht, wird eine Exception ausgelöst */
    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    @Deprecated  // Statt den Scheduler nach Frist zu beenden, soll der Test das bitte selbst machen, sobald alles okay ist. Zschimmer 8.11.2011
    public final void runSchedulerAndTerminate(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        try {
            waitForTermination(timeout);
        } catch (SchedulerRunningAfterTimeoutException x) {
            logger.warn("runSchedulerAndTerminate():"+ x.getMessage());
        }
    }

    @Override public final void startScheduler(String... args) {
        delegate.getEventBus().registerAnnotated(this);
        prepare();
        Iterable<String> allArgs = concat(environment.standardArgs(cppBinaries()), Arrays.asList(args));
        delegate.startScheduler(toArray(allArgs, String.class));
    }

    @Override public final void close() {
        try {
            delegate.getEventBus().unregisterAnnotated(this);
            delegate.close();
            environment.close();
        }
        catch (Throwable x) {
            logger.error(TestSchedulerController.class.getName() + ".close(): " + x);
            throw propagate(x);
        }
    }

    private void prepare() {
        environment.prepare();
        delegate.loadModule(cppBinaries().file(CppBinary.moduleFilename));
    }

    public final Scheduler scheduler() {
        return waitUntilSchedulerIsRunning();
    }

    @Override public final Scheduler waitUntilSchedulerIsRunning() {
        if (scheduler == null) {
            scheduler = delegate.waitUntilSchedulerIsRunning();
            if (terminateOnError) checkForErrorLogLine();
        }
        return scheduler;
    }

    public final void waitForTermination(Time timeout) {
        boolean ok = tryWaitForTermination(timeout);
        if (!ok) throw new SchedulerRunningAfterTimeoutException(timeout);
    }

    private void checkForErrorLogLine() {
        // TODO Prefix_log.last() liefert nur für info die Start-Meldung SCHEDULER-900. Wo sind die anderen Meldungen?
//        String lastErrorLine = scheduler.log().lastByLevel(SchedulerLogLevel.error);
//        if (!lastErrorLine.isEmpty())
//            throw throwErrorLogException(lastErrorLine);
    }

    /** Eine Exception in {@code runnable} beendet den Scheduler. */
    public final Thread newThread(Runnable runnable) {
        ThreadTerminationHandler h = new ThreadTerminationHandler() {
            @Override public void onThreadTermination(@Nullable Throwable t) {
                if (t != null) terminateAfterException(t);
            }
        };
        return new Thread(new TestThreadRunnable(runnable, h), runnable.toString());
    }

    public final Environment environment() {
        return environment;
    }

    public static TestSchedulerController of(Package p) {
        return new TestSchedulerController(new ResourcePath(p));
    }
}
