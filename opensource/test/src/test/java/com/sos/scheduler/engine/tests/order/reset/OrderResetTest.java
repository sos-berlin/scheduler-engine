/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.tests.order.reset;

import com.sos.scheduler.engine.data.order.OrderResumedEvent;
import com.sos.scheduler.engine.data.order.OrderSuspendedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.Test;

public class OrderResetTest extends SchedulerTest {

	private final String JOB_CHAIN = "chain";

	@SuppressWarnings("unused")
	private static final Logger logger = Logger.getLogger(OrderResetTest.class);
    
    private int touchedCount = 0;

	@Test
	public void test() throws Exception {
        controller().setTerminateOnError(false);
		controller().activateScheduler();
        CommandBuilder cmd = new CommandBuilder();
        cmd.addOrder("chain","test");
		scheduler().executeXml(cmd.getCommand());
        controller().waitForTermination(shortTimeout);
	}

    @EventHandler
    public void handleSuspend(OrderSuspendedEvent e) throws InterruptedException {
        // reset the order via modify_order must trigger an OrderResumeEvent
        String cmd = "<modify_order job_chain='/chain' order='test' action='reset'/>";
        scheduler().executeXml(cmd);
    }

    @EventHandler
    public void handleResume(OrderResumedEvent e) throws InterruptedException {
        controller().terminateScheduler();
    }

}
