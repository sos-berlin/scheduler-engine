package com.sos.scheduler.engine.tests.jira.js606.standard;

import java.io.File;
import java.io.IOException;

import com.sos.scheduler.engine.test.util.FileUtils;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.tests.jira.js606.JS606Base;

/**
 * this class tests the default prefix "SCHEDULER_PARAM_"
 * 
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 10.01.2012 10:09:42
 *
 */
public class JS606StandardTest extends JS606Base {

	private static final Logger logger = Logger.getLogger(JS606StandardTest.class);
	
	private final String jobchain = OperatingSystem.isWindows ? "windows_job_chain" : "unix_job_chain";

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS606StandardTest.class.getName());
	}
	
	@Test
	public void Test() throws InterruptedException, IOException {
		controller().activateScheduler();
		prepareTest(jobchain);
		startOrder();
		controller().tryWaitForTermination(shortTimeout);
	}
	

}
