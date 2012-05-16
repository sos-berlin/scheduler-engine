/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.tests.excluded.ss.schedulerparameter;

import com.sos.scheduler.engine.data.job.TaskEndedEvent;
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
public class SchedulerParameterTest extends SchedulerTest {

    private static final Logger logger = Logger.getLogger(SchedulerParameterTest.class);

    private CommandBuilder cmd = new CommandBuilder();

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + SchedulerParameterTest.class.getName());
    }

    @Test
    public void test() throws Exception {
        controller().setTerminateOnError(true);
        controller().activateScheduler("-e", "-env=SCHEDULER_DATA=C:/ProgramData/sos-berlin.com/jobscheduler/scheduler-1.3.12.2093-boerse");
        Scheduler s = scheduler();
        String command = cmd.startJobImmediately("parameter_test").getCommand();
        s.executeXml(command);
        controller().waitForTermination(shortTimeout);
    }

    @EventHandler
    public void handleTaskEnd(TaskEndedEvent e) throws IOException {
        controller().terminateScheduler();
    }


}
