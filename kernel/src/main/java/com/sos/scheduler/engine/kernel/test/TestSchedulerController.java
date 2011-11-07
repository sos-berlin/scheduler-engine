package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Throwables.propagate;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;
import static com.sos.scheduler.engine.kernel.test.binary.TestCppBinaries.cppBinaries;

import java.util.Arrays;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.AnnotatedEventSubscriber;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.event.OperationCollector;
import com.sos.scheduler.engine.kernel.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.main.SchedulerState;
import com.sos.scheduler.engine.kernel.main.SchedulerThreadController;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.test.binary.CppBinary;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.kernel.util.Time;

public class TestSchedulerController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(TestSchedulerController.class);
    public static final Time shortTimeout = Time.of(10);

    private final Environment environment;
    private final SchedulerThreadController delegated = new SchedulerThreadController();
    private boolean terminateOnError = true;
    private Scheduler scheduler = null;

    public TestSchedulerController(ResourcePath resourcePath) {
        environment = new Environment(resourcePath);
    }

    @Override public final void setSettings(Settings o) {
        delegated.setSettings(o);
    }

    /** Bricht den Test mit Fehler ab, wenn ein {@link com.sos.scheduler.engine.kernel.log.ErrorLogEvent} ausgelöst worden ist. */
    public final void setTerminateOnError(boolean o) {
        terminateOnError = o;
    }

    public final void subscribeForAnnotatedEventHandlers(final EventHandlerAnnotated annotated) {
        // TODO Umstellen auf Guava-EventBus und Code vereinfachen
        strictSubscribeEvents(new EventSubscriber() {
            private AnnotatedEventSubscriber subscriber = null;

            @Override public void onEvent(Event e) throws Exception {
                if (subscriber == null && e instanceof SchedulerReadyEvent) {
                    OperationCollector collector = ((SchedulerReadyEvent)e).getScheduler().getEventSubsystem().getOperationCollector();
                    subscriber = AnnotatedEventSubscriber.of(annotated, collector);
                }
                if (subscriber == null)
                    logger.error("Early event ignored: "+e);
                else
                    subscriber.onEvent(e);
            }
        });
    }

    public final void strictSubscribeEvents() {
        strictSubscribeEvents(EventSubscriber.empty);
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        subscribeEvents(s);
    }

    @Override public final void subscribeEvents(EventSubscriber s) {
        delegated.subscribeEvents(new StrictEventSubscriber(s, delegated));
    }

    /** @param timeout Wenn ab Bereitschaft des Schedulers mehr Zeit vergeht, wird eine Exception ausgelöst */
    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    @Override public final void startScheduler(String... args) {
        prepare();
        handleTerminateOnError();
        Iterable<String> allArgs = concat(environment.standardArgs(cppBinaries()), Arrays.asList(args));
        delegated.startScheduler(toArray(allArgs, String.class));
    }

    private void handleTerminateOnError() {
        if (terminateOnError) {
            subscribeEvents(new EventSubscriber() {
                @Override public void onEvent(Event e) throws Exception {
                    if (e instanceof ErrorLogEvent)
                        throw throwErrorLogException(((ErrorLogEvent)e).getMessage().toString());
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

    @Override public final void waitUntilSchedulerState(SchedulerState s) {
        delegated.waitUntilSchedulerState(s);
    }

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
        if (!ok)  throw new RuntimeException("Scheduler has not been terminated within "+timeout);
    }

    @Override public final boolean tryWaitForTermination(Time timeout) {
        return delegated.tryWaitForTermination(timeout);
    }

    @Override public final int exitCode() {
        return delegated.exitCode();
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

    public final Environment environment() {
        return environment;
    }

    public static TestSchedulerController of(Package p) {
        return new TestSchedulerController(new ResourcePath(p));
    }
}
