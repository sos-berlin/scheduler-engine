package com.sos.scheduler.engine.playground.ss.packageload;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.IOException;

public final class ExtractResourcesTest extends SchedulerTest {

    private static final Logger logger = Logger.getLogger(ExtractResourcesTest.class);

    private static final String testChain = "extract_resources";

    @Test public void test() throws Exception {
        controller().activateScheduler("-e");
        CommandBuilder c = new CommandBuilder();
        String command = c.addOrder(testChain)
                .addParam("classname", "com.sos.scheduler.engine.tests.stress.parallelorder.OrderParallelTest")
                .getCommand();
        controller().scheduler().executeXml(command);
        controller().waitForTermination(shortTimeout);
    }

    @EventHandler
    public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
        controller().terminateScheduler();
    }

}
