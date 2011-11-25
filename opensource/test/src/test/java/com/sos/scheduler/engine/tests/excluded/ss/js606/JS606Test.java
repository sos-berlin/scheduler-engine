package com.sos.scheduler.engine.tests.excluded.ss.js606;

import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.HashMap;

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.io.Files;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.tests.excluded.ss.js746.JS746Test;

public class JS606Test extends SchedulerTest {

	private static final Logger logger = LoggerFactory.getLogger(JS606Test.class);
	
	private HashMap<String,Resultset> resultsets = new HashMap<String,Resultset>();
	
	private int orderCount = 0;
	private class Resultset {
		File resultfile;
		String variablePrefixEnv;
		String variablePrefix = "";
		protected Resultset(File f, String p) {
			resultfile = f;
			variablePrefixEnv = p;
			if (!p.equals("*NONE")) variablePrefix = p;
		}
	}

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS746Test.class.getName());
	}
	
	@Test
	public void myTest() throws InterruptedException, IOException {
		controller().startScheduler("-e -env=SCHEDULER_VARIABLE_NAME_PREFIX=MYPREFIX_");
//		doJobchain("node_parameter_1", "");					// Scheduler use default SCHEDULER_PARAM_
//		doJobchain("node_parameter_2", "*NONE");
		doJobchain("node_parameter_3", "MYPREFIX_");
	}
	
	private void doJobchain(String jobchainName, String prefix) {
		Resultset s = prepareResultset(jobchainName, prefix);
//		Environment e = JS606Environment.getInstance(s.variablePrefixEnv);
//		for (String key : e.keySet() ) 
//			logger.debug(key + "=" + e.get(key));
		startOrder(jobchainName);
	}
	
	private void startOrder(String jobchainName) {

		/*
		 * hier sollte eine Order im code erzeugt werden, leider ist nicht klar, wie man die Order dazu bringt,
		 * das sie gestartet wird.
		JobChain jobchain = controller().scheduler().getOrderSubsystem().jobChain(new AbsolutePath("/node_parameter"));
		Order o = jobchain.order( new OrderId("test_node_parameter") );
		o.getParameters().put("", "");
		 */
		
		
		String command = 
		"<add_order id='test_" + jobchainName + "' job_chain='" + jobchainName + "'>" +
		"<params>" + 
		"<param name='RESULT_FILE'    value='" + getResultfile(jobchainName) + "' />" + 
		"<param name='100/NODE_PARAM' value='param_state_100' />" + 
		"<param name='200/NODE_PARAM' value='param_state_200' />" + 
		"</params>" + 
		"</add_order>";
		controller().scheduler().executeXml(command);
	}
	
	private final Resultset prepareResultset(String jobchainName, String prefix) {
		File f = new File( getResultfile(jobchainName) );
		if (f.exists()) f.delete();
		Resultset s = new Resultset(f,prefix);
		resultsets.put(jobchainName, s );
		logger.debug("results of the jobs will be written in file " + f.getAbsolutePath());
		return s;
	}
	
	private String getResultfile(String jobchainName) {
		return "src/test/resources/" + JS606Test.class.getPackage().getName().replace(".", "/") + "/result_" + jobchainName + ".txt";
	}
	
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
		Resultset s = resultsets.get(e.getOrder().getJobChain().getName());
		String lines = Files.toString(s.resultfile, Charset.defaultCharset());
		logger.debug("resultfile is " + s.resultfile.getName() + "\n"+ lines);
		assertParameter(lines, "SCHEDULER_VARIABLE_NAME_PREFIX", s.variablePrefix );
		assertParameter(lines, "JOB1_RESULT", "param_state_100" );
		assertParameter(lines, "JOB2_RESULT", "param_state_200" );
		
		orderCount++;
		if (orderCount == 3) {
			controller().terminateScheduler();
			for (String key : resultsets.keySet()) {
				s = resultsets.get(key);
				if (s.resultfile.exists()) s.resultfile.delete();
			}
		}
	}
	
	private void assertParameter(String content, String paramName, String expectedValue) {
		assertTrue(paramName + "=" + expectedValue + " expected.",content.contains(paramName + "=" + expectedValue));
	}
	

}
