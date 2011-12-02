package com.sos.scheduler.engine.tests.excluded.ss.simple;

import java.io.IOException;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;

public class SimpleTest extends SchedulerTest {

	private static final Logger logger = LoggerFactory.getLogger(SimpleTest.class);
	
	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + SimpleTest.class.getName());
	}

	@Test
	public void test() throws InterruptedException {
        controller().activateScheduler("-e","-log-level=debug");
        startOrder("jobchain1");
	}

	protected void startOrder(String jobchainName) {
		String command = 
		"<add_order id='test_" + jobchainName + "' job_chain='" + jobchainName + "'/>";
		controller().scheduler().executeXml(command);
	}
	
	@HotEventHandler
	public void handleEvent(Event e) throws IOException {
		logger.debug("EVENT: " + e.getClass().getSimpleName());
	}
	
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException, InterruptedException {
		logger.debug("ORDERFINISHED: " + e.getOrder().getId());
		if (e.getOrder().getId().equals("test_jobchain1")) 
			controller().terminateScheduler();
		
	}

}
