package com.sos.scheduler.engine.tests.jira.js793;

import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;

import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import com.google.common.io.Files;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSFileUtils;

/**
 * This test show how to set specific parameter for jobchain nodes. To do this, you have to sepcify order parameters
 * with the state of the jobchain_node for which the parameter you want to set following by a slash (<l>/</l>).
 * 
 * The jobchain <l>windows_node_parameter</l> (resp. <l>windows_node_parameter</l>) consists auf two steps (state <l>100</l> and state <l>200</l>
 * calling the same job <l>shell-job</l>. An order with the following parameter will be started
 * if you run the test:
 * <ul>
 * <li>100/NODE_PARAM = param_state_100</li>
 * <li>200/NODE_PARAM = param_state_200</li>
 * </ul>
 * 
 * This is feature is only available for shell jobs.
 * 
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 25.11.2011 11:01:20
 *
 */
public class JS793Test extends SchedulerTest {

	private static final Logger logger = Logger.getLogger(JS793Test.class);

	private File resultfile;

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS793Test.class.getName());
	}
	
	@Test
	public void myTest() throws InterruptedException, IOException {
		controller().startScheduler();
		if (OperatingSystem.isWindows) {
			doJobchain("windows_node_parameter");
		} else {
			doJobchain("unix_node_parameter");
		}
        controller().tryWaitForTermination(shortTimeout);
	}
	
	private void doJobchain(String jobchainName) throws IOException {
		resultfile = prepareResultfile(jobchainName);
		startOrder(jobchainName);
	}
	
	private void startOrder(String jobchainName) throws IOException {

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
		"<param name='RESULT_FILE'    value='" + getResultfile(jobchainName).getAbsolutePath() + "' />" + 
		"<param name='100/NODE_PARAM' value='param_state_100' />" + 
		"<param name='200/NODE_PARAM' value='param_state_200' />" + 
		"</params>" + 
		"</add_order>";
		controller().scheduler().executeXml(command);
	}
	
	private final File prepareResultfile(String jobchainName) {
		File f = getResultfile(jobchainName);
		if (f.exists()) f.delete();
		logger.debug("results of the jobs will be written in file " + f.getAbsolutePath());
		return f;
	}
	
	private File getResultfile(String jobchainName) {
		return JSFileUtils.getTestresultFile(this.getClass(), "result_" + jobchainName + ".txt");
	}
		
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException {

		String lines = Files.toString(resultfile, Charset.defaultCharset());
		logger.debug("resultfile is " + resultfile.getName() + "\n"+ lines);
		assertParameter(lines, "JOB_RESULT", "param_state_100" );
		assertParameter(lines, "JOB_RESULT", "param_state_200" );
		
		controller().terminateScheduler();
		if (resultfile.exists()) resultfile.delete();
	}
	
	private void assertParameter(String content, String paramName, String expectedValue) {
		assertTrue(paramName + "=" + expectedValue + " expected.",content.contains(paramName + "=" + expectedValue));
	}
	

}
