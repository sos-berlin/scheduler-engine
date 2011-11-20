package com.sos.scheduler.engine.kernelcpptest.scheduler.properties;

import static com.google.common.base.Strings.nullToEmpty;
import org.apache.log4j.Logger;
import org.junit.Test;

import com.sos.scheduler.engine.test.SchedulerTest;

public class PropertiesTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(PropertiesTest.class);

    @Test public void test1() throws Exception {
        try {
            controller().startScheduler();
            System.out.println("port=" + scheduler().getTcpPort());
            System.out.println("host=" + scheduler().getHostname());
            System.out.println("host_complete=" + scheduler().getHostnameLong());
//            fail("Exception expected");
            controller().terminateScheduler();
        }
        catch (Exception x) {
            if (!nullToEmpty(x.getMessage()).contains("SOS-1300"))
                throw new RuntimeException("Exception enthält nicht SOS-1300", x);
            logger.debug("OK: " + x);
        }
    }
}
