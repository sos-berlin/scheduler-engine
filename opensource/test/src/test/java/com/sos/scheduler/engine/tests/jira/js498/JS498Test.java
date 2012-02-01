package com.sos.scheduler.engine.tests.jira.js498;


import static org.junit.Assert.assertTrue;
import java.io.IOException;
import java.lang.reflect.Array;
import java.util.Arrays;
import java.util.List;

import org.junit.Ignore;
import org.junit.Test;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.*;

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
public class JS498Test extends SchedulerTest {

	private final String jobchain = "chain_rhino";
	private final JSCommandBuilder util = new JSCommandBuilder();
	private VariableSet resultSet;

	/*
	 * Unter linux gibt es Probleme beim Zugriff auf den Scheduler-Objekten untegeordneten Objekte, so führt
	 * beispielsweise der Zugriff auf spooler_task.order() zum Absturz dem JVM.
	 * Unter Windows (lokal) funktioniert er.
	 * Der Test wurde deshalb zunächst deaktiviert.
	 */
	@Ignore
	public void testFunctions() throws InterruptedException, IOException {
		controller().activateScheduler("-e","-log-level=info","-log=" + JSFileUtils.getLocalPath(this.getClass()) + "/scheduler.log");
		controller().scheduler().executeXml( util.addOrder(jobchain).getCommand() );
		controller().waitForTermination(shortTimeout);
		testAssertions();
	}
	
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
		resultSet = scheduler().getVariables();
		controller().terminateScheduler();
	}
	
	public void testAssertions() throws IOException {
		assertObject("spooler.variables.count","2");
		assertObject("spooler_task.order.job_chain.name","chain_rhino");
		assertObject("spooler_task.params.names","taskparam1;taskparam2");
		assertObject("spooler_job.order_queue.length","1");
		assertObject("spooler_task.order.id","test_chain_rhino");
		assertFunction("spooler_init");
		assertFunction("spooler_open");
		assertFunction("spooler_process");
		assertFunction("spooler_close");
		assertFunction("spooler_on_success");
		assertFunction("spooler_exit");
		assertFunction("spooler_task_before");
		assertFunction("spooler_task_after");
		assertFunction("spooler_process_before");
		assertFunction("spooler_process_after");
	}

	/**
	 * checks if an estimated funtion was called.
	 * @param chain
	 * @param content
	 * @param function
	 */
	private void assertFunction(String function) {
		assertObject(function,"1");		// any funtion should be called exactly one time
	}
	
	/**
	 * checks if an estimated object was given
	 * @param chain
	 * @param content
	 * @param function
	 */
	private void assertObject(String varname, String expected) {
		String value = resultSet.get(varname);
		assertTrue(varname + " is not set in scheduler variables", value != null);
		assertTrue(value + " is not valid - " + expected + " expected", value.equals(expected));
	}

}
