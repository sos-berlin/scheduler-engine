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

/**
 * \file ProcessListTest.java
 * \brief test functions for the JobScheduler Object Model
 * 
 * \class ProcessListTest
 * \brief test functions for the JobScheduler Object Model
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
