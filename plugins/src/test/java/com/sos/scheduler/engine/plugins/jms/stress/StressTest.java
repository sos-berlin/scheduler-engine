package com.sos.scheduler.engine.plugins.jms.stress;

import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Ignore;

import java.io.File;

import static org.junit.Assert.assertEquals;

/**
 * This class is a stress test for JobScheduler. You can determine the number of tasks and the runtime of each
 * task with ESTIMATED_TASKS and JOB_RUNTIME_IN_SECONDS.
 * Please note the JobScheduler installation supports 200 maximum parallel processes at the time. To run
 * a test with more parallel processes you you to change max_processes in spooler.h or use the binaries 
 * of version 1.3.12.1346-BT.
 */
public class StressTest extends SchedulerTest implements TaskInfoListener {

    private static final Logger logger = Logger.getLogger(StressTest.class);
    private final CommandBuilder util = new CommandBuilder();

	private static final String jobName = OperatingSystem.isWindows ? "job_windows" : "job_unix";
//    private static final String providerUrl = "tcp://w2k3.sos:61616";
    private static final String providerUrl = "vm://localhost";

    private int taskFinished = 0;
	
	private static final int ESTIMATED_TASKS = 100;
	private static final int JOB_RUNTIME_IN_SECONDS = 1;
	private static final Time MAX_RUNTIME = Time.of(60);

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + StressTest.class.getName());
	}
	
	@Ignore
	public void eventTest() throws Exception {
//        controller().activateScheduler("-e -log-level=debug","-log=" + FileUtils.getLocalFile(this.getClass(), "scheduler.log"));
        controller().activateScheduler();
        File resultFile = new File (scheduler().getConfiguration().logDirectory() + "/result.csv");
        // File resultFile = new File(FileUtils.getResourceFile("").getAbsolutePath() + "/result.csv");
        logger.info("resultfile is " + resultFile);
        JMSTaskObserver l = new JMSTaskObserver(providerUrl);
        TaskObserverWriter w = new TaskObserverWriter(resultFile.getAbsolutePath(),l,ESTIMATED_TASKS);
        l.addListener(this);
        l.addListener(w);
        w.start(1000L,1000L);
		for (int i=0; i < ESTIMATED_TASKS; i++) {
			controller().scheduler().executeXml(
					util.startJobImmediately(jobName)
					.addParam("DELAY", String.valueOf(JOB_RUNTIME_IN_SECONDS))
					.getCommand());
		}
        controller().waitForTermination(MAX_RUNTIME);
        assertEquals("only " + l.endedTasks() + " are finished - " + ESTIMATED_TASKS + " estimated.",ESTIMATED_TASKS,l.endedTasks());
        w.stop();
	}
	
    @EventHandler
    public void handleTaskEnd(TaskEndedEvent e) throws Exception {
    	logger.debug("TASKENDED: " + e);
    	taskFinished++;
    	if (taskFinished == ESTIMATED_TASKS)
    		controller().scheduler().terminate();
    }

	@Override
	public void onInterval(TaskInfo info) {
		logger.info(info.currentlyRunningTasks() + " tasks running");
	}
    
}
