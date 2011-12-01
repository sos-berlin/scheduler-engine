package com.sos.scheduler.engine.tests.events;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.test.SchedulerTest;

public class OrderEventTest extends SchedulerTest {

	private static Logger logger;
	private int count = 0;
	private Time timeout = Time.of(20);

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		logger = LoggerFactory.getLogger(OrderEventTest.class);
		logger.info("Starting test of EventPlugin");
	}

	@Test
	public void test() throws InterruptedException {
		controller().startScheduler();
		controller().waitForTermination(timeout);
	}

    /*
      * Es wird erwartet, dass 8 Order beendet werden (vgl. scheduler.xml), erst dann darf der Scheduler beendet werden
      */
    @HotEventHandler
	public void handleEvent(OrderFinishedEvent e) {
		count++;
		if (count == 8)
			controller().terminateScheduler();
	}

	@HotEventHandler
	public void handleEvent2(OrderEvent e) {
		logger.info(e.getClass().getSimpleName() + " for order " + e.getKey().getId() );
	}

}
