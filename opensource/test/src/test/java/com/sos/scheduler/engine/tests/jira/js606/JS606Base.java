package com.sos.scheduler.engine.tests.jira.js606;

import com.google.common.io.Files;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;

import static org.junit.Assert.assertTrue;

public abstract class JS606Base extends SchedulerTest {

	private static final Logger logger = Logger.getLogger(JS606Base.class);
	
	private File resultfile;
	private String jobchainName;
	private final CommandBuilder util = new CommandBuilder();
	
	
	protected void prepareTest(String jobchain) throws IOException {
		
		this.jobchainName = jobchain;

		resultfile = File.createTempFile("result_" + jobchainName, ".txt");
		logger.debug("results of the jobs will be written in file " + resultfile.getAbsolutePath());
	}
	
	protected void startOrder() {
		util.addOrder(jobchainName)
			.addParam("RESULT_FILE", resultfile.getAbsolutePath())
			.addParam("ORDER_PARAM", "ORDER_PARAM")
		;
		controller().scheduler().executeXml(util.getCommand());
	}
	
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
		String lines = Files.toString(resultfile, Charset.defaultCharset());
		logger.debug("resultfile is " + resultfile.getName() + "\n"+ lines);
		assertParameter(lines, "RESULT_FILE", resultfile.getAbsolutePath() );
		assertParameter(lines, "ORDER_PARAM", "ORDER_PARAM" );
		assertParameter(lines, "JOB_PARAM", "JOB_PARAM" );
		controller().terminateScheduler();
	}
	
	private void assertParameter(String content, String paramName, String expectedValue) {
		assertTrue(paramName + "=" + expectedValue + " expected.",content.contains(paramName + "=" + expectedValue));
	}
	

}
