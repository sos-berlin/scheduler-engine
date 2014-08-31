package com.sos.scheduler.engine.tests.order.reset;

import com.sos.scheduler.engine.data.order.OrderResumedEvent;
import com.sos.scheduler.engine.data.order.OrderSuspendedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.configuration.TestConfigurationBuilder;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.Test;

public class OrderResetIT extends SchedulerTest {

	private final String JOB_CHAIN = "chain";

    private int touchedCount = 0;

    public OrderResetIT() {
        super(new TestConfigurationBuilder(OrderResetIT.class).terminateOnError(false).build());
    }

	@Test
	public void test() throws Exception {
		controller().activateScheduler();
        CommandBuilder cmd = new CommandBuilder();
        cmd.addOrder("chain","test");
		scheduler().executeXml(cmd.getCommand());
        controller().waitForTermination();
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
