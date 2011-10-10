package com.sos.scheduler.engine.kernel.test;

import java.io.File;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.TemporaryFolder;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest {
    public static final Time shortTimeout = Time.of(10);

    @Rule public final TemporaryFolder folder = new TemporaryFolder();
    private TestSchedulerController controller = null;

    protected SchedulerTest() {}

    protected SchedulerTest(Iterable<String> configFilenames) {
        this();
    }

    @Before public final void schedulerTestBefore() {
        controller = TestSchedulerController.of(getClass().getPackage(), folder);
    }

    @After public final void terminateAndCleanUp() throws Throwable {
        controller.terminateAndCleanUp();
    }

    public final void strictSubscribeEvents() {
        controller.strictSubscribeEvents();
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        controller.strictSubscribeEvents(s);
    }

    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    public final void startScheduler(String... args) {
        controller.startScheduler(args);
    }

    public final void waitUntilSchedulerIsRunning() {
        controller.waitUntilSchedulerIsRunning();
    }

    public final Scheduler scheduler() {
        return controller.scheduler();
    }

    public final void waitForTermination(Time timeout) {
        controller.waitForTermination(timeout);
    }

    public final TestSchedulerController controller() {
        return controller;
    }

    public final File getDirectory() {
        return controller.directory();
    }
}
