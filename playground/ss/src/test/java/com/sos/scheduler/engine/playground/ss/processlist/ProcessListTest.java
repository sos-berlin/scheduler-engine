package com.sos.scheduler.engine.playground.ss.processlist;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.IOException;

@SuppressWarnings("deprecation")
public class ProcessListTest extends SchedulerTest {

    private static final Logger logger = Logger.getLogger(ProcessListTest.class);

    private CommandBuilder cmd = new CommandBuilder();

    @Test
    public void test() throws Exception {
        controller().setTerminateOnError(true);
        controller().activateScheduler("-e");
        String command = cmd.addOrder("processlist_test").getCommand();
        logger.info("command: " + command);
        controller().scheduler().executeXml(command);
        controller().waitForTermination(shortTimeout);
    }

    @EventHandler
    public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
        controller().terminateScheduler();
    }

}
