package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Strings.isNullOrEmpty;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;

import java.io.File;
import java.util.Arrays;

import org.apache.log4j.Logger;
import org.junit.After;
import org.junit.rules.TemporaryFolder;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.main.SchedulerThreadController;
import com.sos.scheduler.engine.kernel.util.Lazy;
import com.sos.scheduler.engine.kernel.util.Time;

public class TestSchedulerController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(TestSchedulerController.class);

    private final SchedulerController delegate;
    private final Environment env;
    private Scheduler scheduler = null;

    public TestSchedulerController(Package pack, Lazy<File> testDirectory) {
        delegate = new SchedulerThreadController();
        env = new Environment(pack, testDirectory);
    }

    public final void strictSubscribeEvents() {
        strictSubscribeEvents(EventSubscriber.empty);
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        delegate.subscribeEvents(new StrictEventSubscriber(s));
    }

    @Override public void subscribeEvents(EventSubscriber s) {
        delegate.subscribeEvents(s);
    }

    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    public final void startScheduler(String... args) {
        env.start();
        Iterable<String> allArgs = concat(env.standardArgs(), Arrays.asList(args));
        delegate.startScheduler(toArray(allArgs, String.class));
    }

    public final Scheduler scheduler() {
        if (scheduler == null)
            waitUntilSchedulerIsRunning();
        assert scheduler != null;
        return scheduler;
    }

    public final Scheduler waitUntilSchedulerIsRunning() {
        scheduler = delegate.waitUntilSchedulerIsRunning();
        return scheduler;
    }

    @Override public void terminateScheduler() {
        delegate.terminateScheduler();
    }

    @Override public void terminateAfterException(Throwable x) {
        delegate.terminateAfterException(x);
    }

    @Override public void terminateAndWait() {
        delegate.terminateAndWait();
    }

    @Override public final void waitForTermination(Time timeout) {
        delegate.waitForTermination(timeout);
    }

    @Override public int exitCode() {
        return delegate.exitCode();
    }

    @After public final void terminateAndCleanUp() throws Throwable {
        try {
            delegate.terminateAndWait();
        }
        catch (Throwable x) {
            logger.error(TestSchedulerController.class.getName() + " @After: " + x, x);
            throw x;
        }
    }

    public final File directory() {
        return env.getDirectory();
    }

    public final SchedulerController delegate() {
        return delegate;
    }

    public static TestSchedulerController of(Package pack, final TemporaryFolder temporaryFolder) {
        Lazy<File> dirLazy = new Lazy<File>() {
            public File compute() {
                assert temporaryFolder.getRoot() != null : "TemporaryFolder.@Before has not been executed?";
                String prop = System.getProperty("test.scheduler.dir");
                return isNullOrEmpty(prop)? temporaryFolder.getRoot() : new File(prop);
            }
        };
        return new TestSchedulerController(pack, dirLazy);
    }
}
