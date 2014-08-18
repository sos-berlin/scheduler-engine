package com.sos.scheduler.engine.test;

import com.google.common.io.Files;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.test.configuration.TestConfiguration;
import com.sos.scheduler.engine.test.configuration.TestConfigurationBuilder;
import org.joda.time.Duration;
import org.junit.After;
import org.junit.Before;

import java.io.File;
import java.io.IOException;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    public static final Duration shortTimeout = TestSchedulerController.shortTimeout();
    public static final Duration errorOnlyTimeout = TestSchedulerController.errorOnlyTimeout();

    private final TestSchedulerController controller;

    protected SchedulerTest() {
        controller = TestSchedulerController.apply(new TestConfigurationBuilder(getClass()).build());
    }

    protected SchedulerTest(TestConfiguration c) {
        controller = TestSchedulerController.apply(c);
    }

    protected SchedulerTest(TestSchedulerController controller) {
        this.controller = controller;
    }

    @Before public final void schedulerTestBefore() {
        controller.getEventBus().registerAnnotated(this);
    }

    @After public final void schedulerTestClose() {
        controller.getEventBus().unregisterAnnotated(this);
        controller.close();
    }

    public final TestSchedulerController controller() {
        return controller;
    }

    public final <T> T instance(Class<T> c) {
        return scheduler().injector().getInstance(c);
    }

    /** Zur Bequemlichkeit; dasselbe wie {@link com.sos.scheduler.engine.test.TestSchedulerController#scheduler()}. */
    protected final Scheduler scheduler() {
        return controller().scheduler();
    }

    public final File getTempFile(String fileWithoutPath) {
        File resultFile = new File(controller.environment().directory().getAbsolutePath(), fileWithoutPath);
        try {
            Files.createParentDirs(resultFile);
        } catch (IOException e) {
            throw new RuntimeException("Error creating directory: " + resultFile, e);
        }
        return resultFile;
    }
}
