package com.sos.scheduler.engine.plugins.event.stepevents;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.kernel.test.SuperSchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.plugins.event.Connector;


public class OrderStepEventsTest extends SuperSchedulerTest {

    private static final Time schedulerTimeout = Time.of(15);
    private static Logger logger;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
		logger = LoggerFactory.getLogger(Connector.class);
		logger.info("Starting test of EventPlugin");
	}
    
    @Test
    public void test() throws Exception {
        runScheduler(schedulerTimeout, "-e -log-level=warn");
    }
    
}
