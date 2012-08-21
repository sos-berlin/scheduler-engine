package com.sos.scheduler.engine.tests.jira.js606.empty;

import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.tests.jira.js606.JS606Base;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.IOException;

/**
 * This class tests an empty prefix for JobScheduler environment variables (see scheduler.xml)
 */
public class JS606EmptyTest extends JS606Base {

	private static final Logger logger = Logger.getLogger(JS606EmptyTest.class);
	
	private final String jobchain = OperatingSystem.isWindows ? "windows_job_chain" : "unix_job_chain";

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS606EmptyTest.class.getName());
	}
	
	@Test
	public void test() throws InterruptedException, IOException {
		controller().activateScheduler();
		prepareTest(jobchain);
		startOrder();
		controller().tryWaitForTermination(shortTimeout);
	}
	

}
