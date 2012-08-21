package com.sos.scheduler.engine.tests.jira.js606.standard;

import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.tests.jira.js606.JS606Base;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.IOException;

/**
 * this class tests the default prefix "SCHEDULER_PARAM_" for JobScheduler environment variables
 */
public class JS606StandardTest extends JS606Base {

	private static final Logger logger = Logger.getLogger(JS606StandardTest.class);
	
	private final String jobchain = OperatingSystem.isWindows ? "windows_job_chain" : "unix_job_chain";

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS606StandardTest.class.getName());
	}
	
	@Test
	public void test() throws InterruptedException, IOException {
		controller().activateScheduler();
		prepareTest(jobchain);
		startOrder();
		controller().tryWaitForTermination(shortTimeout);
	}
	

}
