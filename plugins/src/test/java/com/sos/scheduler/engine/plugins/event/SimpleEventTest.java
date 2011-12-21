package com.sos.scheduler.engine.plugins.event;

import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;

public class SimpleEventTest extends SchedulerTest {

    private static final Time schedulerTimeout = Time.of(20);
    private static Logger logger;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
		logger = Logger.getLogger(Connector.class);
		logger.info("Starting test of EventPlugin");
	}
    
    @Test
    public void test() throws Exception {
        controller().runSchedulerAndTerminate(schedulerTimeout, "-e -log-level=warn");
    }
    
}
