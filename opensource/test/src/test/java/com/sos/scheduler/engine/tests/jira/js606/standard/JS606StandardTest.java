package com.sos.scheduler.engine.tests.jira.js606.standard;

import java.io.IOException;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.tests.jira.js606.JS606Base;

public class JS606StandardTest extends JS606Base {

	private static final Logger logger = LoggerFactory.getLogger(JS606StandardTest.class);
	
	private final String prefix = "";
	private final String jobchain = OperatingSystem.isWindows ? "windows_node_parameter" : "unix_node_parameter";

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS606StandardTest.class.getName());
	}
	
	@Test
	public void Test() throws InterruptedException, IOException {
		controller().activateScheduler("-env=SCHEDULER_VARIABLE_NAME_PREFIX=" + prefix);
		prepareTest(jobchain, prefix);
		startOrder();
		controller().tryWaitForTermination(shortTimeout);
	}
	

}
