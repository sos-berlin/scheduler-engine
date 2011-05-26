package com.sos.scheduler.engine.kernelcpptest.scheduler.properties;

import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import org.apache.log4j.*;
import org.junit.*;
import static org.junit.Assert.*;


public class PropertiesTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(PropertiesTest.class);

    @Test public void test1() throws Exception {
        try {
            runScheduler(shortTimeout);
            System.out.println("port=" + scheduler.getTcpPort());
            System.out.println("host=" + scheduler.getHostname());
            System.out.println("host_complete=" + scheduler.getHostnameLong());
//            fail("Exception expected");
        }
        catch (Exception x) {
            if (!x.getMessage().contains("SOS-1300"))  fail("Exception enthält nicht SOS-1300");
            logger.debug("OK: " + x);
        }
    }
}
