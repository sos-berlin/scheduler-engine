package com.sos.scheduler.engine.kernel.test;

import java.io.File;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.TemporaryFolder;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest {
    public static final Time shortTimeout = Time.of(10);

    @Rule public final TemporaryFolder folder = new TemporaryFolder();
    private TestSchedulerController testSchedulerController;
    protected SchedulerController schedulerController;
    protected Scheduler scheduler = null;

    protected SchedulerTest() {}

    protected SchedulerTest(Iterable<String> configFilenames) {
        this();
    }

    @Before public final void schedulerTestBefore() {
        testSchedulerController = TestSchedulerController.of(getClass().getPackage(), folder);
        schedulerController = testSchedulerController.controller();
    }

    public final void strictSubscribeEvents() {
        testSchedulerController.strictSubscribeEvents();
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        testSchedulerController.strictSubscribeEvents(s);
    }

    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    public final void startScheduler(String... args) {
        testSchedulerController.startScheduler(args);
    }

    public final void waitUntilSchedulerIsRunning() {
        testSchedulerController.waitUntilSchedulerIsRunning();
    }

    public final Scheduler getScheduler() {
        return testSchedulerController.scheduler();
    }

    public final void waitForTermination(Time timeout) {
        testSchedulerController.waitForTermination(timeout);
    }

    @After public final void terminateAndCleanUp() throws Throwable {
        testSchedulerController.terminateAndCleanUp();
    }

    public final SchedulerController controller() {
        return schedulerController;
    }

    public final File getDirectory() {
        return testSchedulerController.directory();
    }
}
