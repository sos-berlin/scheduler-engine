package com.sos.scheduler.engine.plugins.event;

import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.folder.events.FileBasedActivatedEvent;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSCommandUtils;

public class SimpleEventTest extends SchedulerTest {

	private final static Logger logger = Logger.getLogger(SimpleEventTest.class);
	private final static JSCommandUtils utils = JSCommandUtils.getInstance();
	private final static String jobChain = "EventPluginTest";

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		logger.debug("Starting test of EventPlugin");
	}

	@Test
	public void test() throws Exception {
		controller().activateScheduler();
		controller().scheduler().executeXml(
				utils.buildCommandAddOrder(jobChain,jobChain).addParam("myParam", "EventPluginTest").getCommand());
		controller().waitForTermination(shortTimeout);
	}
	
    @HotEventHandler
    public void handleOrderEnd(OrderFinishedEvent e, UnmodifiableOrder o) throws Exception {
    	logger.debug("ORDERFINISHED: " + o.getId().getString());
    	if (o.getId().getString().equals(jobChain)) {
    		Thread.sleep(3000);
    		controller().scheduler().terminate();
    	}
    }

}
