package com.sos.scheduler.engine.tests.excluded.ss.orderreset;

import java.io.IOException;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;

public class OrderResetTest extends SchedulerTest {

	private static final Logger logger = LoggerFactory.getLogger(OrderResetTest.class);
	
	private final String jobchain = "chain1";

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + OrderResetTest.class.getName());
	}
	
	@Test
	public void Test() throws InterruptedException, IOException {
		controller().startScheduler("-e");
		startOrder(jobchain);
	}
	
	protected void startOrder(String jobchainName) {

		/*
		 * hier sollte eine Order im code erzeugt werden, leider ist nicht klar, wie man die Order dazu bringt,
		 * das sie gestartet wird.
		JobChain jobchain = controller().scheduler().getOrderSubsystem().jobChain(new AbsolutePath("/node_parameter"));
		Order o = jobchain.order( new OrderId("test_node_parameter") );
		o.getParameters().put("", "");
		 */
		
		
		String command = 
		"<add_order id='test_" + jobchainName + "' job_chain='" + jobchainName + "'>" +
		"<params>" + 
		"<param name='100/NODE_PARAM' value='param_state_100' />" + 
		"</params>" + 
		"</add_order>";
		controller().scheduler().executeXml(command);
	}
	
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
		logger.debug("order finished");
		/*
		assertParameter(lines, "SCHEDULER_VARIABLE_NAME_PREFIX", variablePrefixEnv );
		assertParameter(lines, "JOB_RESULT", "param_state_100" );
		assertParameter(lines, "JOB_RESULT", "param_state_200" );
		 */
		
		controller().terminateScheduler();
		
	}
	
//	private void assertParameter(String content, String paramName, String expectedValue) {
//		assertTrue(paramName + "=" + expectedValue + " expected.",content.contains(paramName + "=" + expectedValue));
//	}
	

}
