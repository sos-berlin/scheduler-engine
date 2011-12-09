package com.sos.scheduler.engine.tests.excluded.js721;

import static org.junit.Assert.fail;

import java.io.File;
import java.io.IOException;
import java.util.Timer;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent;
import com.sos.scheduler.engine.kernel.job.events.TaskStartedEvent;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.tests.excluded.ss.scripttest.JavaXScriptTest;

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
public class JS721Test extends SchedulerTest implements TaskInfo {

	private static final Logger logger = LoggerFactory.getLogger(JS721Test.class);
	
	private static final String jobName = OperatingSystem.isWindows ? "job_windows" : "job_unix";
	private static final int ESTIMATED_TASKS = 10;
	private static final int JOB_RUNTIME_IN_SECONDS = 1;
	
	private int endedTasks = 0;
	private int runningTasks = 0;
	private int maxParallelTasks = 0;
	private TaskObserver loggerTask = null;
	
	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS721Test.class.getName());
	}

	@Test
	public void test() throws Exception {
		
        controller().activateScheduler();
        
		String logfilename = "src/test/java/" + JS721Test.class.getPackage().getName().replace(".", "/") + "/JS721.log";
		File f = new File( logfilename );
		Timer timer = new Timer();
		loggerTask = new TaskObserver(f.getAbsolutePath(), this);

        String params = "<params><param name='DELAY' value='" + JOB_RUNTIME_IN_SECONDS + "' /></params>";
		try {
			timer.schedule(loggerTask, 1000, 1000);
	        for (int i = 0; i < ESTIMATED_TASKS; i++) {
	            startJob(jobName,params);
	        }
		} finally {
	        controller().tryWaitForTermination(shortTimeout);
	        if (endedTasks != ESTIMATED_TASKS)
	        	fail(endedTasks + " are finished yet - " + ESTIMATED_TASKS + " are estimated");
	        logger.info("max. " + maxParallelTasks + " tasks ran parallel.");
			loggerTask.close();
			timer.cancel();
			timer.purge();
		}
        
	}

	protected void startJob(String jobName, String params) {
		String command = "<start_job job='" + jobName + "' at='now'>" + params + "</start_job>";
		controller().scheduler().executeXml(command);
	}
	
	@HotEventHandler
	public void handleTaskStart(TaskStartedEvent e) throws IOException {
		runningTasks++;
		if (runningTasks > maxParallelTasks) maxParallelTasks = runningTasks;
		logger.debug("TASKSTART: " + e.getClass().getSimpleName());
	}

	@HotEventHandler
	public void handleTaskEnd(TaskEndedEvent e) throws IOException {
		logger.debug("TASKEND: " + e.getClass().getSimpleName());
		runningTasks--;
		endedTasks++;
		if (endedTasks == ESTIMATED_TASKS)
			controller().terminateScheduler();
	}

	@Override
	public int currentlyRunningTasks() {
		return runningTasks;
	}

	@Override
	public int endedTasks() {
		return endedTasks;
	}

	@Override
	public int highwaterTasks() {
		return maxParallelTasks;
	}

	@Override
	public int estimatedTasks() {
		return ESTIMATED_TASKS;
	}
	
}
