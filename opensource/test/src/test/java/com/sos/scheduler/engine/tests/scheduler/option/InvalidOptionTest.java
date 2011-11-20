package com.sos.scheduler.engine.tests.scheduler.option;

import static org.junit.Assert.fail;

import org.apache.log4j.Logger;
import org.junit.Test;

import com.sos.scheduler.engine.test.SchedulerTest;


public class InvalidOptionTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(InvalidOptionTest.class);


    @Test public void test1() throws Exception {
        try {
            controller().runScheduler(shortTimeout, "-INVALID");
            fail("Exception expected");
        }
        catch (Exception x) {
            if (!x.getMessage().contains("SOS-1300"))  fail("Exception enthält nicht SOS-1300");
            logger.debug("OK: " + x);
        }
    }
}
