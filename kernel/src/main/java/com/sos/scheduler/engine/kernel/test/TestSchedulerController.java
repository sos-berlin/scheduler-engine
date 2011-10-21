package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Throwables.propagate;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;
import static com.sos.scheduler.engine.kernel.test.TestSchedulerCppBinaries.*;
import static com.sos.scheduler.engine.kernel.util.Files.makeTemporaryDirectory;
import static com.sos.scheduler.engine.kernel.util.Files.tryRemoveDirectoryRecursivly;

import java.io.File;
import java.util.Arrays;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.main.SchedulerThreadController;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.kernel.util.Time;

public class TestSchedulerController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(TestSchedulerController.class);
    public static final Time shortTimeout = Time.of(10);

    private final SchedulerThreadController delegated;
    private final Environment environment;
    private final File binDirectory;
    private Scheduler scheduler = null;

    public TestSchedulerController(ResourcePath resourcePath) {
        delegated = new SchedulerThreadController();
        environment = new Environment(resourcePath);
        binDirectory = makeTemporaryDirectory();
    }

    public final void strictSubscribeEvents() {
        strictSubscribeEvents(EventSubscriber.empty);
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        delegated.subscribeEvents(new StrictEventSubscriber(s));
    }

    @Override public final void subscribeEvents(EventSubscriber s) {
        delegated.subscribeEvents(s);
    }

    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    @Override public final void startScheduler(String... args) {
        prepare();
        Iterable<String> allArgs = concat(environment.standardArgs(cppBinaries()), Arrays.asList(args));
        delegated.startScheduler(toArray(allArgs, String.class));
    }

    private void prepare() {
        environment.prepare();
        delegated.loadModule(cppBinaries().moduleFile());
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
            tryRemoveDirectoryRecursivly(binDirectory);
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
