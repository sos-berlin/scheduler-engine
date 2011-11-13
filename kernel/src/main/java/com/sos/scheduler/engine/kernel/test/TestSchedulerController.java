package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Throwables.propagate;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;
import static com.sos.scheduler.engine.kernel.test.binary.TestCppBinaries.cppBinaries;

import java.util.Arrays;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.eventbus.AnnotatedEventSubscribers;
import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventHandlerFailedEvent;
import com.sos.scheduler.engine.eventbus.EventSubscriber2;
import com.sos.scheduler.engine.eventbus.EventSubscriber2Adapter;
import com.sos.scheduler.engine.eventbus.GenericEventSubscriber;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.main.SchedulerState;
import com.sos.scheduler.engine.kernel.main.SchedulerThreadController;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.test.binary.CppBinary;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.kernel.util.Time;

public class TestSchedulerController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(TestSchedulerController.class);
    public static final Time shortTimeout = Time.of(10);

    private final EventBus strictEventBus = new EventBus();
    private final Environment environment;
    private final SchedulerThreadController delegated = new SchedulerThreadController();
    private boolean terminateOnError = true;
    private Scheduler scheduler = null;

    public TestSchedulerController(ResourcePath resourcePath) {
        environment = new Environment(resourcePath);
        delegated.getEventBus().register(new GenericEventSubscriber<Event>(Event.class) {
            @Override protected void handleTypedEvent(Event e) {
                strictEventBus.publishImmediately(e);
            }
        });
        strictEventBus.register(new GenericEventSubscriber<EventHandlerFailedEvent>(EventHandlerFailedEvent.class) {
            @Override protected void handleTypedEvent(EventHandlerFailedEvent e) {
                terminateAfterException(e.getThrowable());
            }
        });
    }

    public final void close() {
        try {
            delegated.terminateAndWait();
            environment.close();
        }
        catch (Throwable x) {
            logger.error(TestSchedulerController.class.getName() + ".close(): " + x);
            throw propagate(x);
        }
    }

    @Override public final void setSettings(Settings o) {
        delegated.setSettings(o);
    }

    /** Bricht den Test mit Fehler ab, wenn ein {@link com.sos.scheduler.engine.kernel.log.ErrorLogEvent} ausgelöst worden ist. */
    public final void setTerminateOnError(boolean o) {
        terminateOnError = o;
    }

    public final void subscribeForAnnotatedEventHandlers(EventHandlerAnnotated annotated) {
        strictSubscribeEvents(AnnotatedEventSubscribers.handlers(annotated));
    }

//    public final void strictSubscribeEvents() {
//        strictSubscribeEvents(EventSubscriber.empty);
//    }

    private void strictSubscribeEvents(Iterable<EventSubscriber2> subscribers) {
        for (EventSubscriber2 s: subscribers) subscribeEvents(s);
    }

    public final void strictSubscribeEvents(EventSubscriber2 s) {
        subscribeEvents(s);
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        strictSubscribeEvents(new EventSubscriber2Adapter(s));
    }

    private void subscribeEvents(EventSubscriber s) {
        subscribeEvents(new EventSubscriber2Adapter(s));
    }

    public final void subscribeEvents(EventSubscriber2 s) {
        strictEventBus.register(s);
    }

    /** @param timeout Wenn ab Bereitschaft des Schedulers mehr Zeit vergeht, wird eine Exception ausgelöst */
    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    @Deprecated  // Der Test soll den Scheduler explizit beenden, wenn alles okay ist. Zschimmer 8.11.2011
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
        prepare();
        handleTerminateOnError();
        Iterable<String> allArgs = concat(environment.standardArgs(cppBinaries()), Arrays.asList(args));
        delegated.startScheduler(toArray(allArgs, String.class));
    }

    private void handleTerminateOnError() {
        if (terminateOnError) {
            subscribeForAnnotatedEventHandlers(new EventHandlerAnnotated() {
                @EventHandler public void handleEvent(ErrorLogEvent e) {
                    throw throwErrorLogException(e.getMessage().toString());
                }
            });
        }
    }

    private void prepare() {
        environment.prepare();
        delegated.loadModule(cppBinaries().file(CppBinary.moduleFilename));
    }

    public final Scheduler scheduler() {
        return waitUntilSchedulerIsRunning();
    }

    @Override public final Scheduler waitUntilSchedulerIsRunning() {
        if (scheduler == null) {
            scheduler = delegated.waitUntilSchedulerIsRunning();
            if (terminateOnError) checkForErrorLogLine();
        }
        return scheduler;
    }

    private void checkForErrorLogLine() {
        // TODO Prefix_log.last() liefert nur für info die Start-Meldung SCHEDULER-900. Wo sind die anderen Meldungen?
//        String lastErrorLine = scheduler.log().lastByLevel(SchedulerLogLevel.error);
//        if (!lastErrorLine.isEmpty())
//            throw throwErrorLogException(lastErrorLine);
    }

    private static RuntimeException throwErrorLogException(String errorLine) {
        throw new AssertionError(errorLine);
    }

    /** Eine Exception in {@code runnable} beendet den Scheduler. */
    public final Thread newThread(Runnable runnable) {
        ThreadTerminationHandler threadTerminationHandler =  new ThreadTerminationHandler() {
            @Override public void onThreadTermination(@Nullable Throwable t) {
                if (t != null) terminateAfterException(t);
            }
        };
        Thread result = new Thread(new TestThreadRunnable(runnable, threadTerminationHandler));
        result.setName(runnable.toString());
        return result;
    }

    @Override public final void waitUntilSchedulerState(SchedulerState s) {
        delegated.waitUntilSchedulerState(s);
    }

//    @Override public SchedulerState getSchedulerState() {
//        return delegated.getSchedulerState();
//    }

    @Override public final void terminateScheduler() {
        delegated.terminateScheduler();
    }

    @Override public final void terminateAfterException(Throwable x) {
        delegated.terminateAfterException(x);
    }

    @Override public final void terminateAndWait() {
        delegated.terminateAndWait();
    }

    public final void waitForTermination(Time timeout) {
        boolean ok = tryWaitForTermination(timeout);
        if (!ok) throw new SchedulerRunningAfterTimeoutException(timeout);
    }

    @Override public final boolean tryWaitForTermination(Time timeout) {
        return delegated.tryWaitForTermination(timeout);
    }

    @Override public final int exitCode() {
        return delegated.exitCode();
    }

    @Override public EventBus getEventBus() {
        return delegated.getEventBus();
    }

    public final Environment environment() {
        return environment;
    }

    public static TestSchedulerController of(Package p) {
        return new TestSchedulerController(new ResourcePath(p));
    }
}
