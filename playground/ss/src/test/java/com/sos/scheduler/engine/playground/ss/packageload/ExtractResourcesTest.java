package com.sos.scheduler.engine.playground.ss.packageload;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.Test;

import java.io.IOException;

public final class ExtractResourcesTest extends SchedulerTest {

    private static final String testChain = "extract_resources";

    @Test public void test() throws Exception {
        controller().activateScheduler("-e");
        CommandBuilder c = new CommandBuilder();
        String command = c.addOrder(testChain)
                .addParam("classname", "com.sos.scheduler.engine.tests.stress.parallelorder.OrderParallelIT")
                .getCommand();
        controller().scheduler().executeXml(command);
        controller().waitForTermination();
    }

    @EventHandler
    public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
        controller().terminateScheduler();
    }

}