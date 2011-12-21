package com.sos.scheduler.engine.tests.jira.js606;

import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.io.Files;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSFileUtils;

public class JS606Base extends SchedulerTest {

	private static final Logger logger = LoggerFactory.getLogger(JS606Base.class);
	
	private String variablePrefixEnv;
	private File resultfile;
	private String jobchainName;
	
	
	protected void prepareTest(String jobchain, String prefix) {
		
		this.variablePrefixEnv = prefix;
		this.jobchainName = jobchain;

		resultfile = JSFileUtils.getEmptyTestresultFile(this.getClass(), "result_" + jobchainName + ".txt");
		logger.debug("results of the jobs will be written in file " + resultfile.getAbsolutePath());
	}
	
	protected void startOrder() {

		/*
		 * hier sollte eine Order im code erzeugt werden, leider ist nicht klar, wie man die Order dazu bringt,
		 * das sie gestartet wird.
		JobChain jobchain = controller().scheduler().getOrderSubsystem().jobChain(new AbsolutePath("/node_parameter"));
		Order o = jobchain.order( new OrderId("test_node_parameter") );
		o.getParameters().put("", "");
		 */
		
		
		String command = 
		"<add_order id='test_" + jobchainName + "' job_chain='" + this.jobchainName + "'>" +
		"<params>" + 
		"<param name='RESULT_FILE'    value='" + resultfile.getAbsolutePath() + "' />" + 
		"<param name='100/NODE_PARAM' value='param_state_100' />" + 
		"<param name='200/NODE_PARAM' value='param_state_200' />" + 
		"</params>" + 
		"</add_order>";
		controller().scheduler().executeXml(command);
	}
	
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
		String lines = Files.toString(resultfile, Charset.defaultCharset());
		logger.debug("resultfile is " + resultfile.getName() + "\n"+ lines);
		assertParameter(lines, "SCHEDULER_VARIABLE_NAME_PREFIX", variablePrefixEnv );
		assertParameter(lines, "JOB_RESULT", "param_state_100" );
		assertParameter(lines, "JOB_RESULT", "param_state_200" );
		
		controller().terminateScheduler();
		
	}
	
	private void assertParameter(String content, String paramName, String expectedValue) {
		assertTrue(paramName + "=" + expectedValue + " expected.",content.contains(paramName + "=" + expectedValue));
	}
	

}
