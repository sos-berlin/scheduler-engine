package com.sos.scheduler.engine.tests.excluded.js721;

import static org.junit.Assert.fail;

import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.joda.time.DateTime;
import org.joda.time.Duration;
import org.joda.time.format.DateTimeFormat;
import org.joda.time.format.DateTimeFormatter;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent;
import com.sos.scheduler.engine.kernel.job.events.TaskStartedEvent;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.test.SchedulerTest;

/**
 * This test is for running a lot of simultaniously processes in one instance of the JObScheduler.
 * A simple job is started ESTIMATED_TASKS times. The job only waits 
 * 
 * This behaviour to run more than simulataniously processes is only available and tested for shell jobs.
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 08.12.2011 13:57:27
 *
 */
public class JS721Test extends SchedulerTest {

	private static final Logger logger = LoggerFactory.getLogger(JS721Test.class);
	
	private static final String jobName = OperatingSystem.isWindows ? "job_windows" : "job_unix";
	private static final int ESTIMATED_TASKS = 100;
	private static final int JOB_RUNTIME_IN_SECONDS = 1;
	
	private int endedTasks = 0;
	private int runningTasks = 0;
	private int maxParallelTasks = 0;
	
	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS721Test.class.getName());
	}

	@Test
	public void test() throws InterruptedException {
        controller().activateScheduler();
        String params = "<params><param name='DELAY' value='1' /></params>";
        for (int i = 0; i < ESTIMATED_TASKS; i++) {
            startJob(jobName,params);
        }
        controller().tryWaitForTermination(shortTimeout);
        
        if (endedTasks != ESTIMATED_TASKS)
        	fail(endedTasks + " are finished yet - " + ESTIMATED_TASKS + " are estimated");
	}

	protected void startJob(String jobName, String params) {
		String command = "<start_job job='" + jobName + "' at='now'>" + params + "</start_job>";
		controller().scheduler().executeXml(command);
	}
	
	@HotEventHandler
	public void handleTaskStart(TaskStartedEvent e) throws IOException {
		runningTasks++;
		if (runningTasks > maxParallelTasks) maxParallelTasks = runningTasks;
		logger.debug(e.getJobPath().getString() + " TASKSTART: " + e.getClass().getSimpleName());
		logStatus();
	}

	@HotEventHandler
	public void handleTaskEnd(TaskEndedEvent e) throws IOException {
		logger.debug(e.getJobPath().getString() + " TASKEND: " + e.getClass().getSimpleName());
		runningTasks--;
		endedTasks++;
		logStatus();
		if (endedTasks == ESTIMATED_TASKS)
			controller().terminateScheduler();
	}
	
	private void logStatus() {
		logger.info(runningTasks + " are currently running (" + endedTasks + " of " + ESTIMATED_TASKS + " are finished)");
	}

}
