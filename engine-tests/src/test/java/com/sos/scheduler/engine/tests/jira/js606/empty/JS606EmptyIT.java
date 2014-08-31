package com.sos.scheduler.engine.tests.jira.js606.empty;

import com.sos.scheduler.engine.common.system.OperatingSystem;
import com.sos.scheduler.engine.tests.jira.js606.JS606Base;
import org.junit.Test;

import java.io.IOException;

/**
 * This class tests an empty prefix for JobScheduler environment variables (see scheduler.xml)
 */
public class JS606EmptyIT extends JS606Base {

	private final String jobchain = OperatingSystem.isWindows ? "windows_job_chain" : "unix_job_chain";

	@Test
	public void test() throws InterruptedException, IOException {
		controller().activateScheduler();
		prepareTest(jobchain);
		startOrder();
		controller().waitForTermination();
	}
}
