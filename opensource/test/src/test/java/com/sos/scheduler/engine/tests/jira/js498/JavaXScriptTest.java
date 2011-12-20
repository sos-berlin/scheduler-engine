package com.sos.scheduler.engine.tests.jira.js498;


import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.io.Files;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.scheduler.events.SchedulerCloseEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSTestUtils;

/**
 * This is a test for scripting with the Rhino engine. The jobchain chain_scripting executes a job with all knwowing API
 * methods (including the monitors). At the end of the test the resultfile will checked for the estimated functions calls.
 * 
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 16.12.2011 13:39:41
 *
 */
public class JavaXScriptTest extends SchedulerTest {

	private final List<String> jobchains = Arrays.asList("chain_rhino");
	
	private JSTestUtils util = JSTestUtils.getInstance();
	private HashMap<String,File> resultfiles = new HashMap<String,File>();
	
	private int finishedOrders = 0;

	@Test
	public void Test() throws InterruptedException, IOException {
		controller().startScheduler("-e","-log-level=debug");
		for (String jobchain : jobchains) {
			File resultfile = JSTestUtils.getEmptyTestresultFile(this.getClass(), jobchain + ".log");
			resultfiles.put(jobchain, resultfile);
			util.addParam("resultfile", resultfile.getAbsolutePath());
			controller().scheduler().executeXml( util.buildCommandAddOrder(jobchain) );
		}
		controller().tryWaitForTermination(shortTimeout);
	}
	
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
		finishedOrders++;
		if (finishedOrders == jobchains.size())
			controller().terminateScheduler();
		
	}
	
	@HotEventHandler
	public void testAssertions(SchedulerCloseEvent e) throws IOException {
		for (String chain : resultfiles.keySet()) {
			String lines = Files.toString(resultfiles.get(chain), Charset.defaultCharset());
			assertFunction(chain, lines,"spooler_init");
			assertFunction(chain, lines,"spooler_open");
			assertFunction(chain, lines,"spooler_process");
			assertFunction(chain, lines,"spooler_close");
			assertFunction(chain, lines,"spooler_on_success");
			assertFunction(chain, lines,"spooler_exit");
			assertFunction(chain, lines,"spooler_task_before");
			assertFunction(chain, lines,"spooler_task_after");
			assertFunction(chain, lines,"spooler_process_before");
			assertFunction(chain, lines,"spooler_process_after");
		}
	}
	
	private void assertFunction(String chain, String content, String function) {
		assertTrue(chain + ": " + function + " not executed.",content.contains(function));
	}

}
