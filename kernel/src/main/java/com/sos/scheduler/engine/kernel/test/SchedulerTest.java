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
    private SchedulerTestDriver driver;
    protected SchedulerController schedulerController;
    protected Scheduler scheduler = null;

    public SchedulerTest() {}

    @Before public final void schedulerTestBefore() {
        driver = SchedulerTestDriver.of(getClass().getPackage(), folder);
        schedulerController = driver.controller();
    }

    public SchedulerTest(Iterable<String> configFilenames) {
        this();
    }

    public final void strictSubscribeEvents() {
        driver.strictSubscribeEvents();
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        driver.strictSubscribeEvents(s);
    }

    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    public final void startScheduler(String... args) {
        driver.startScheduler(args);
    }

    public final void waitUntilSchedulerIsRunning() {
        driver.waitUntilSchedulerIsRunning();
    }

    public final Scheduler getScheduler() {
        return driver.scheduler();
    }

    public final void waitForTermination(Time timeout) {
        driver.waitForTermination(timeout);
    }

    @After public final void terminateAndCleanUp() throws Throwable {
        driver.terminateAndCleanUp();
    }

    public final SchedulerController controller() {
        return schedulerController;
    }

    public final File getDirectory() {
        return driver.directory();
    }
}
