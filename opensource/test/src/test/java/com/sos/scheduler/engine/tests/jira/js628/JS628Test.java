package com.sos.scheduler.engine.tests.jira.js628;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.*;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 * \file JS628Test.java
 * \brief js-628: Order successfull when a pre/postprocessing script is used
 * 
 * \class JS628Test
 * \brief js-628: Order successfull when a pre/postprocessing script is used
 * 
 * \details
 * This test contain four jobchains for various combinations of the result from spooler_process and
 * spooler_process_before:
 *
 * spooler_process 		spooler_process_before 		result				job_chain
 * ================== 	======================== 	=================	=====================
 * true					false						error				js628_chain_fail_1
 * false				false						error				js628_chain_fail_2
 * false				true						error				js628_chain_fail_3
 * true					true						success				js628_chain_success
 *
 * It should be clarify that the job ends only if the tesult of spooler_process AND spooler_process_before
 * is true.
 *
 * The test estimate that one job_chain ends with success and three job_chains ends with a failure.
 *
 * \author ss
 * \version 1.1 - 27.03.2012 10:45:12
 * <div class="sos_branding">
 * <p>
 * (c) 2011 SOS GmbH - Berlin (<a style='color:silver'
 * href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)
 * </p>
 * </div>
 */
public class JS628Test extends SchedulerTest {

	private final String[] JOB_CHAINS = {"js628-chain-success","js628-chain-fail-1","js628-chain-fail-2","js628-chain-fail-3"};

	@SuppressWarnings("unused")
	private static Logger logger;

	private final CommandBuilder utils = new CommandBuilder();
    private int finishedOrderCount = 0;
    private int errorCount = 0;
    private int successCount = 0;

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		logger = Logger.getLogger(JS628Test.class);
	}

	@Test
	public void test() throws Exception {
		controller().setTerminateOnError(false);
		controller().activateScheduler();
        for(String jobChain : JOB_CHAINS) { 
		    String cmd = utils.addOrder(jobChain).getCommand();
            controller().scheduler().executeXml(cmd);
        }
		controller().waitForTermination(shortTimeout);
        assertEquals("total number of events", JOB_CHAINS.length, finishedOrderCount);
        assertEquals("successfull orders", 1, successCount);
        assertEquals("unsuccessfull orders", 3, errorCount);
	}

	@HotEventHandler
	public void handleEvent(OrderFinishedEvent e, UnmodifiableOrder order) throws InterruptedException {
        String endState = order.getState().getString();
        if (endState.equals("error")) errorCount++;
        if (endState.equals("success")) successCount++;
        finishedOrderCount++;
        if (finishedOrderCount == JOB_CHAINS.length)
			controller().scheduler().terminate();
	}
}
