package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Throwables.propagate;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;
import static com.sos.scheduler.engine.kernel.test.binary.TestCppBinaries.cppBinaries;

import java.util.Arrays;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.main.SchedulerState;
import com.sos.scheduler.engine.kernel.main.SchedulerThreadController;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.test.binary.CppBinary;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.kernel.util.Time;

public class TestSchedulerController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(TestSchedulerController.class);
    public static final Time shortTimeout = Time.of(10);

    private final SchedulerThreadController delegated;
    private final Environment environment;
    private boolean terminateOnError = true;
    private Scheduler scheduler = null;

    public TestSchedulerController(ResourcePath resourcePath, Settings settings) {
        delegated = new SchedulerThreadController(settings);
        environment = new Environment(resourcePath);
    }

    /** Bricht den Test mit Fehler ab, wenn ein {@link com.sos.scheduler.engine.kernel.log.ErrorLogEvent} ausgelöst worden ist. */
    public void setTerminateOnError(boolean o) {
        terminateOnError = o;
    }

    public void subscribeForAnnotatedEventHandlers(final EventHandlerAnnotated annotatedObject) {
        delegated.subscribeEvents(new EventSubscriber() {
            @Override public void onEvent(Event e) throws Exception {
                if (e instanceof SchedulerReadyEvent)
                    scheduler().getEventSubsystem().subscribeAnnotated(annotatedObject);
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
                        throw new AssertionError(((ErrorLogEvent)e).getMessage().toString());
                }
            });
        }
    }

    private void prepare() {
        environment.prepare();
        delegated.loadModule(cppBinaries().file(CppBinary.moduleFilename));
    }

    public final Scheduler scheduler() {
        if (scheduler == null)
            waitUntilSchedulerIsRunning();
        assert scheduler != null;
        return scheduler;
    }

    @Override public final Scheduler waitUntilSchedulerIsRunning() {
        scheduler = delegated.waitUntilSchedulerIsRunning();
        return scheduler;
    }

    @Override public void waitUntilSchedulerState(SchedulerState s) {
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

    @Override public final void waitForTermination(Time timeout) {
        delegated.waitForTermination(timeout);
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
        return of(p, DefaultSettings.singleton);
    }

    public static TestSchedulerController of(Package p, Settings settings) {
        return new TestSchedulerController(new ResourcePath(p), settings);
    }
}
