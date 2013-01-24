package com.sos.scheduler.engine.test.schedulertest;

import static org.junit.Assert.fail;

import org.junit.Test;

import com.sos.scheduler.engine.test.SchedulerTest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** Testet {@link com.sos.scheduler.engine.test.SchedulerTest} */
public final class ErrorLogIT extends SchedulerTest {
    private static final Logger logger = LoggerFactory.getLogger(ErrorLogIT.class);

    @Test public void infoLogLineShouldNotThrowException() {
        controller().activateScheduler();
        scheduler().log().info("TEST-INFO");
        controller().close();
    }

    @Test public void errorLogLineShouldThrowException() {
        controller().activateScheduler();
        scheduler().log().error("TEST-ERROR");
        try {
            controller().close();
            fail("Missing Exception for error log line");
        } catch(Exception x) {
            logger.debug("Okay: {}", x);
        }
    }
}
