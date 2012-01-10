package com.sos.scheduler.engine.tests.jira.js498;


import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import org.apache.log4j.Logger;
import org.junit.Test;

import sos.spooler.Variable_set;

import com.google.common.io.Files;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.scheduler.events.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSCommandUtils;
import com.sos.scheduler.engine.test.util.JSFileUtils;

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
	
	private final JSCommandUtils util = JSCommandUtils.getInstance();
	private VariableSet resultSet;

	/*
	 * Unter linux gibt es Probleme beim Zugriff auf den Scheduler-Objekten untegeordneten Objekte, so führt
	 * beispielsweise der Zugriff auf spooler_task.order() zum Absturz dem JVM.
	 * Unter Windows (lokal) funktioniert er.
	 * Der Test wurde deshalb zunächst deaktiviert.
	 */
	@Test
	public void test() throws InterruptedException, IOException {
		controller().activateScheduler();
		controller().scheduler().executeXml( util.buildCommandAddOrder(jobchain).getCommand() );
		controller().waitForTermination(shortTimeout);
		testAssertions();
	}
	
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
		resultSet = scheduler().getVariables();
		controller().terminateScheduler();
	}
	
	public void testAssertions() throws IOException {
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
		assertObject("spooler","sos.spooler.Process_classes");
		assertObject("spooler","sos.spooler.Variable_set");
		assertObject("spooler_task","sos.spooler.Order");
		assertObject("spooler_task","sos.spooler.Variable_set");
		assertObject("spooler_task","sos.spooler.Subprocess");
		assertObject("spooler_job","sos.spooler.Order_queue");
		assertObject("spooler_job","sos.spooler.Process_class");
	}

	
	/**
	 * checks if an estimated funtion was called.
	 * @param chain
	 * @param content
	 * @param function
	 */
	private void assertFunction(String function) {
		String value = resultSet.get(function);
		assertTrue(function + " is not set in scheduler variables", value != null);
		assertTrue(function + " has to be called exact one time", value.equals("1"));
	}
	
	/**
	 * checks if an estimated (sub-)object was given
	 * @param chain
	 * @param content
	 * @param function
	 */
	private void assertObject(String type, String object) {
		String varname = type + "_" + object;
		String value = resultSet.get(varname);
		assertTrue(varname + " is not set in scheduler variables", value != null);
		assertTrue(value + " is not valid - " + object + " expected", value.startsWith(object + "@"));
	}

}
