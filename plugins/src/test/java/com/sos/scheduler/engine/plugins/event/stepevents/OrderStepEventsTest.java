package com.sos.scheduler.engine.plugins.event.stepevents;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.JSHelper.Logging.Log4JHelper;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.plugins.event.Connector;


public class OrderStepEventsTest extends SchedulerTest {

    private static final Time schedulerTimeout = Time.of(20);
    private static Logger logger;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
		// this file contains appender for ActiveMQ logging
		new Log4JHelper("src/test/resources/log4j.properties");
		logger = LoggerFactory.getLogger(Connector.class);
		logger.info("Starting test of EventPlugin");
	}
    
    @Test
    public void test() throws Exception {
        runScheduler(schedulerTimeout, "-e");
    }
    
}
