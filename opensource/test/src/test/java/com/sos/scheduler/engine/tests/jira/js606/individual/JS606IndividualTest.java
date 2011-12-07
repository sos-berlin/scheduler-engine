package com.sos.scheduler.engine.tests.jira.js606.individual;

import java.io.IOException;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.tests.jira.js606.JS606Base;

public class JS606IndividualTest extends JS606Base {

	private static final Logger logger = LoggerFactory.getLogger(JS606IndividualTest.class);
	
	private final String prefix = "MYPREFIX_";
	private final String jobchain = OperatingSystem.isWindows ? "windows_node_parameter" : "unix_node_parameter";

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS606IndividualTest.class.getName());
	}
	
	@Test
	public void Test() throws InterruptedException, IOException {
		controller().startScheduler("-env=SCHEDULER_VARIABLE_NAME_PREFIX=" + prefix);
		prepareTest(jobchain, prefix);
		startOrder();
	}
	

}
