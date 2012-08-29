package com.sos.scheduler.engine.test;

import com.google.common.io.Files;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.util.Time;
import org.junit.After;
import org.junit.Before;

import java.io.File;
import java.io.IOException;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    public static final Time shortTimeout = TestSchedulerController.shortTimeout;

    private final TestSchedulerController controller;

    protected SchedulerTest(TestSchedulerController controller) {
        this.controller = controller;
    }

    protected SchedulerTest() {
        controller = TestSchedulerController.builder(getClass()).build();
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

    /** Zur Bequemlichkeit; dasselbe wie {@link com.sos.scheduler.engine.test.TestSchedulerController#scheduler()}. */
    protected final Scheduler scheduler() {
        return controller().scheduler();
    }

    public final File getTempFile(Class<?> classInstance, String fileWithoutPath) {
        File resultDir = new File(controller.environment().directory().getAbsolutePath(),classInstance.getName());
        File resultFile = new File(resultDir,fileWithoutPath);
        try {
            Files.createParentDirs(resultFile);
        } catch (IOException e) {
            throw new SchedulerException("error creating directory for file " + resultFile.getAbsolutePath());
        }
        return resultFile;
    }

}
