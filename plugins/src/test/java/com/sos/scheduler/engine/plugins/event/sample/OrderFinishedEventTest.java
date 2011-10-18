package com.sos.scheduler.engine.plugins.event.sample;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.plugins.event.Connector;


public class OrderFinishedEventTest extends SchedulerTest {

    private static final Time schedulerTimeout = Time.of(15);
    private static Logger logger;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
		logger = LoggerFactory.getLogger(Connector.class);
		logger.info("Starting test of EventPlugin");
	}
    
    @Test
    public void test() throws Exception {
        controller().runScheduler(schedulerTimeout, "-e -log-level=warn");
    }
    
}
