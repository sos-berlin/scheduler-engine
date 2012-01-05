package com.sos.scheduler.engine.tests.jira.js461;

import static org.junit.Assert.assertEquals;

import java.util.Iterator;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderResumedEvent;
import com.sos.scheduler.engine.kernel.order.OrderSuspendedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSCommandUtils;

/**
 * \file JS461Test.java
 * \brief js-461: modify order set state to endstate
 * 
 * \class JS461Test
 * \brief js-461: modify order set state to endstate
 * 
 * \details
 * The sample configuration contains a jobchain with three nodes.
 * Running this test the chain starts and should be suspend at the second node
 * (job js-461-2), because the job ends with error. The test set the state of
 * the suspended order to "success" and resumed it.
 * The test expects the following events fired by the scheduler:
 * - EventOrderSuspended if the job job js-461-2 ends with error
 * - EventOrderResumed if the order was set to suspended="no"
 * - EventOrderFinished because the order resumed in the "success" state
 * 
 * \author ss
 * \version 1.1 - 29.12.2011 09:56:21
 * <div class="sos_branding">
 * <p>
 * (c) 2011 SOS GmbH - Berlin (<a style='color:silver'
 * href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)
 * </p>
 * </div>
 */
public class JS461Test extends SchedulerTest {

	private final String JOB_CHAIN = "js-461";

	@SuppressWarnings("unused")
	private static Logger logger;
	private final static JSCommandUtils utils = JSCommandUtils.getInstance();

	// Queue for collecting the fired events in the listener thread
	private final BlockingQueue<String> resultQueue = new ArrayBlockingQueue<String>(50);

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		logger = Logger.getLogger(JS461Test.class);
	}

	@Test
	public void test() throws Exception {
		controller().setTerminateOnError(false);
		controller().activateScheduler();
		utils.buildCommandAddOrder(JOB_CHAIN);
		controller().scheduler().executeXml(utils.getCommand());
		while (numberOfEvents("OrderSuspendedEvent") == 0) {}
		controller().scheduler().executeXml("<modify_order job_chain='/" + JOB_CHAIN + "' order='test_" + JOB_CHAIN + "' state='success'/>");
		controller().scheduler().executeXml("<modify_order job_chain='/" + JOB_CHAIN + "' order='test_" + JOB_CHAIN + "' suspended='no'/>");
		controller().waitForTermination(shortTimeout);
		assertEvent("OrderSuspendedEvent", 1);
		assertEvent("OrderResumedEvent", 1);
		assertEvent("OrderFinishedEvent", 1);
		assertEquals("total number of events", 3, resultQueue.size());
	}

	private void assertEvent(String eventName, int exceptedHits) {
		assertEquals("event '" + eventName + "'", exceptedHits, numberOfEvents(eventName));
	}

	private int numberOfEvents(String eventName) {
		Iterator<String> it = resultQueue.iterator();
		int cnt = 0;
		while (it.hasNext()) {
			String e = it.next();
			if (e.equals(eventName))
				cnt++;
		}
		return cnt;
	}

	@HotEventHandler
	public void handleEvent(OrderEvent e) throws InterruptedException {
		if (e instanceof OrderResumedEvent || e instanceof OrderSuspendedEvent || e instanceof OrderFinishedEvent)
			resultQueue.put(e.getClass().getSimpleName());
		if (e instanceof OrderResumedEvent)
			controller().scheduler().terminate();
	}
}
