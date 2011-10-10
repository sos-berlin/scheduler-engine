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
import com.sos.scheduler.engine.kernel.util.Time;

public class TestSchedulerController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(TestSchedulerController.class);

    private final SchedulerController controller;
    private final Environment env;
    private Scheduler scheduler = null;

    public TestSchedulerController(Package pack, File testDirectory) {
        controller = new SchedulerThreadController();
        env = new Environment(pack, testDirectory);
    }

    public final void strictSubscribeEvents() {
        strictSubscribeEvents(EventSubscriber.empty);
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        controller.subscribeEvents(new StrictEventSubscriber(s));
    }

    @Override public void subscribeEvents(EventSubscriber s) {
        controller.subscribeEvents(s);
    }

    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    public final void startScheduler(String... args) {
        Iterable<String> allArgs = concat(env.standardArgs(), Arrays.asList(args));
        controller.startScheduler(toArray(allArgs, String.class));
    }

    public final Scheduler scheduler() {
        if (scheduler == null)
            waitUntilSchedulerIsRunning();
        assert scheduler != null;
        return scheduler;
    }

    public final Scheduler waitUntilSchedulerIsRunning() {
        scheduler = controller.waitUntilSchedulerIsRunning();
        return scheduler;
    }

    @Override public void terminateScheduler() {
        controller.terminateScheduler();
    }

    @Override public void terminateAfterException(Throwable x) {
        controller.terminateAfterException(x);
    }

    @Override public void terminateAndWait() {
        controller.terminateAndWait();
    }

    @Override public final void waitForTermination(Time timeout) {
        controller.waitForTermination(timeout);
    }

    @Override public int exitCode() {
        return controller.exitCode();
    }

    @After public final void terminateAndCleanUp() throws Throwable {
        try {
            controller.terminateAndWait();
        }
        catch (Throwable x) {
            logger.error(TestSchedulerController.class.getName() + " @After: " + x, x);
            throw x;
        }
    }

    public final File directory() {
        return env.getDirectory();
    }

    public final SchedulerController controller() {
        return controller;
    }

    public static TestSchedulerController of(Package pack, TemporaryFolder f) {
        assert f.getRoot() != null : "TemporaryFolder.@Before has not been executed?";
        String d = System.getProperty("test.scheduler.dir");
        File dir = isNullOrEmpty(d)? f.getRoot() : new File(d);
        return new TestSchedulerController(pack, dir);
    }
}
