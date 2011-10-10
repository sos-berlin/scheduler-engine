package com.sos.scheduler.engine.plugins.event;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.test.SuperSchedulerTest;

public class SimpleEventTest extends SuperSchedulerTest {

    private static final Time schedulerTimeout = Time.of(10);
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
