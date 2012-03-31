package com.sos.scheduler.engine.plugins.event;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

public class SimpleEventTest extends SchedulerTest {

	private final static Logger logger = Logger.getLogger(SimpleEventTest.class);
	private final static String jobChain = "EventPluginTest";
	
	private final CommandBuilder utils = new CommandBuilder();

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		logger.debug("Starting test of EventPlugin");
	}

	@Test
	public void test() throws Exception {
		controller().activateScheduler();
		controller().scheduler().executeXml(
				utils.addOrder(jobChain,jobChain).addParam("myParam", "EventPluginTest").getCommand());
		controller().waitForTermination(shortTimeout);
	}
	
    @HotEventHandler
    public void handleOrderEnd(OrderFinishedEvent e, UnmodifiableOrder o) throws Exception {
    	logger.debug("ORDERFINISHED: " + o.getId().getString());
    	if (o.getId().getString().equals(jobChain))
    		controller().scheduler().terminate();
    }

}
