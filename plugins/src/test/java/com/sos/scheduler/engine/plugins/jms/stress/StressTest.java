package com.sos.scheduler.engine.plugins.jms.stress;

import static org.junit.Assert.assertEquals;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSCommandUtils;

public class StressTest extends SchedulerTest {

    private static final Logger logger = Logger.getLogger(StressTest.class);
    private static final JSCommandUtils util = JSCommandUtils.getInstance();

	private static final String jobName = OperatingSystem.isWindows ? "job_windows" : "job_unix";
//    private static final String providerUrl = "tcp://w2k3.sos:61616";
    private static final String providerUrl = "vm://localhost";

    private int taskFinished = 0;
	
	private static final int ESTIMATED_TASKS = 10;
	private static final int JOB_RUNTIME_IN_SECONDS = 1;
	private static final Time MAX_RUNTIME = Time.of(30);

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + StressTest.class.getName());
	}
	
	@Test
	public void eventTest() throws Exception {
//        controller().activateScheduler("-e -log-level=debug","-log=" + JSFileUtils.getLocalFile(this.getClass(), "scheduler.log"));
        controller().activateScheduler();
        TaskObserverPlugin l = TaskObserverPlugin.getInstance(providerUrl,ESTIMATED_TASKS);
        l.start(1000L,1000L);
		for (int i=0; i < ESTIMATED_TASKS; i++) {
			controller().scheduler().executeXml(
					util.buildCommandStartJobImmediately(jobName)
					.addParam("DELAY", String.valueOf(JOB_RUNTIME_IN_SECONDS))
					.getCommand());
		}
        controller().waitForTermination(MAX_RUNTIME);
        assertEquals("only " + l.endedTasks() + " are finished - " + ESTIMATED_TASKS + " estimated.",ESTIMATED_TASKS,l.endedTasks());
        l.stop();
	}
	
    @HotEventHandler
    public void handleTaskEnd(TaskEndedEvent e, UnmodifiableTask t) throws Exception {
    	logger.debug("TASKENDED: " + t.getId());
    	taskFinished++;
    	if (taskFinished == ESTIMATED_TASKS)
    		controller().scheduler().terminate();
    }
    
}
