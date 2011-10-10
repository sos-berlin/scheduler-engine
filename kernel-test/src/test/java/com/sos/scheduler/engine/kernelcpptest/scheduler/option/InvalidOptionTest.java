package com.sos.scheduler.engine.kernelcpptest.scheduler.option;

import com.sos.scheduler.engine.kernel.test.SuperSchedulerTest;
import org.apache.log4j.*;
import org.junit.*;
import static org.junit.Assert.*;


public class InvalidOptionTest extends SuperSchedulerTest {
    private static final Logger logger = Logger.getLogger(InvalidOptionTest.class);


    @Test public void test1() throws Exception {
        try {
            runScheduler(shortTimeout, "-INVALID");
            fail("Exception expected");
        }
        catch (Exception x) {
            if (!x.getMessage().contains("SOS-1300"))  fail("Exception enthält nicht SOS-1300");
            logger.debug("OK: " + x);
        }
    }
}
