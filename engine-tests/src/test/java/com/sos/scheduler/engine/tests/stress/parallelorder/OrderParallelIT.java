package com.sos.scheduler.engine.tests.stress.parallelorder;

import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.order.OrderFinished;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import java.time.Duration;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public final class OrderParallelIT extends SchedulerTest {

    private static final Logger logger = LoggerFactory.getLogger(OrderParallelIT.class);

    // In Maven setzen mit -DargLine=-DOrderParallelTest.limit=26 -DOrderParallelTest.runtime=5 (Surefire plugin 2.6), 2010-11-28
    // Zum Beispiel: mvn test -Dtest=ExtractResourcesTest -DargLine="-DOrderParallelTest.limit=26  -DOrderParallelTest.runtime=5"
    private static final int testLimit = Integer.parseInt(System.getProperty("ExtractResourcesTest.limit", "10"));
    private static final int testRuntime = Integer.parseInt(System.getProperty("ExtractResourcesTest.runtime", "5"));
    private static final String testChain = System.getProperty("ExtractResourcesTest.jobchain", "start_job");
    private static final int total = testLimit + 1;          // +1 is for the initializing order 'order_creator'
    private static final Duration timeOut = Duration.ofSeconds(300);

    private int finishedOrdersCount = 0;

    @Test public void test() throws Exception {
        controller().activateScheduler();
        CommandBuilder c = new CommandBuilder();
        String command = c.addOrder("order_creator")
                .addParam("RUNTIME_IN_SECONDS", String.valueOf(testRuntime))
                .addParam("NUMBER_OF_ORDERS",String.valueOf(testLimit))
                .addParam("CHAIN_TO_START",testChain)
                .getCommand();
        controller().scheduler().executeXml(command);
        controller().waitForTermination(timeOut);
    }

    @EventHandler
    public void handleOrderEnd(KeyedEvent<OrderFinished> g) {
        finishedOrdersCount++;
        if (finishedOrdersCount > 1)
            logger.info("Order " + (finishedOrdersCount-1) + " of " + testLimit + " finished");
        if (finishedOrdersCount == total)
            controller().terminateScheduler();
    }
}
