/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.tests.stress.parallelorder;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.IOException;

public final class OrderParallelTest extends SchedulerTest {

    private static final Logger logger = Logger.getLogger(OrderParallelTest.class);

    // In Maven setzen mit -DargLine=-DOrderParallelTest.limit=26 -DOrderParallelTest.runtime=5 (Surefire plugin 2.6), 2010-11-28
    // Zum Beispiel: mvn test -Dtest=ExtractResourcesTest -DargLine="-DOrderParallelTest.limit=26  -DOrderParallelTest.runtime=5"
    private static final int testLimit = Integer.parseInt(System.getProperty("ExtractResourcesTest.limit", "10"));
    private static final int testRuntime = Integer.parseInt(System.getProperty("ExtractResourcesTest.runtime", "5"));
    private static final String testChain = System.getProperty("ExtractResourcesTest.jobchain", "start_job");
    private static final int total = testLimit + 1;          // +1 is for the initializing order 'order_creator'
    private static final Time timeOut = Time.of(300);

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
    public void handleOrderEnd(OrderFinishedEvent e) {
        finishedOrdersCount++;
        if (finishedOrdersCount > 1)
            logger.info("order " + (finishedOrdersCount-1) + " of " + testLimit + " finished." );
        if (finishedOrdersCount == total)
            controller().terminateScheduler();
    }

}
