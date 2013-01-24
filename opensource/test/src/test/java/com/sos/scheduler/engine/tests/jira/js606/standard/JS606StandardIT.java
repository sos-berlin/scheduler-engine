package com.sos.scheduler.engine.tests.jira.js606.standard;

import com.sos.scheduler.engine.common.system.OperatingSystem;
import com.sos.scheduler.engine.tests.jira.js606.JS606Base;
import org.junit.Test;

import java.io.IOException;

/**
 * this class tests the default prefix "SCHEDULER_PARAM_" for JobScheduler environment variables
 */
public class JS606StandardIT extends JS606Base {

	private final String jobchain = OperatingSystem.isWindows ? "windows_job_chain" : "unix_job_chain";

	@Test
	public void test() throws InterruptedException, IOException {
		controller().activateScheduler();
		prepareTest(jobchain);
		startOrder();
		controller().tryWaitForTermination(shortTimeout);
	}
}
