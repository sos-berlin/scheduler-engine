package com.sos.scheduler.engine.plugins.event;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;

public class SimpleEventTest extends SchedulerTest {

    private static final Time schedulerTimeout = Time.of(10);
    private static Logger logger;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
		logger = LoggerFactory.getLogger(Connector.class);
		logger.info("Starting test of EventPlugin");
	}
    
    @Test
    public void test() throws Exception {
        controller().runSchedulerAndTerminate(schedulerTimeout, "-e -log-level=warn");
    }
    
}
