package com.sos.jobnet.reference;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.IOException;

/**
 * \file ReferenceNetTest.java
 * \brief test for the reference jobnet
 * 
 * \class ReferenceNetTest
 * \brief test for the reference jobnet
 * 
 * \details
 *
 * \code
   \endcode
 * 
 * \author ss 
 * \version 1.0 - 19.04.2012 10:53:10
 * <div class="sos_branding">
 * <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
@SuppressWarnings("deprecation")
public class ReferenceNetTest extends SchedulerTest {

    //TODO mit geeigneten Assertion die Korrektheit der Ausführung prüfen.

    private static final Logger logger = Logger.getLogger(ReferenceNetTest.class);

    private static final int totalNumberOfOrders = 8;    // 1 for JobNetPlanExecutor, 7 for Jobnet
    private int numberOfFinishedOrders = 0;
    private static final Time timeout = Time.of(300);
    private CommandBuilder cmd = new CommandBuilder();

    @Test
    public void test() throws Exception {
        controller().activateScheduler();

        // This file connects with Oracle DB at 8of9
        String hibernateConfig = controller().scheduler().getConfiguration().localConfigurationDirectory() + "/hibernate.cfg.xml";
        scheduler().getVariables().put("hibernate_connection_config_file",hibernateConfig);

        String command = cmd.addOrder("jobnet_plan_creator", "reference").
                addParam("JobChainName", "reference").
                addParam("OrderId", "A").
                getCommand();
        scheduler().executeXml(command);
        controller().waitForTermination(timeout);
    }

    @EventHandler
    public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
        logger.info("order " + e.getKey().toString() + " ended.");
        numberOfFinishedOrders++;
        if (numberOfFinishedOrders == totalNumberOfOrders)
            controller().terminateScheduler();
    }


}
