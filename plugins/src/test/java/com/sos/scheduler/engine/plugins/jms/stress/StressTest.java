package com.sos.scheduler.engine.plugins.jms.stress;

import static org.junit.Assert.assertEquals;

import java.io.File;

import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSCommandBuilder;
import com.sos.scheduler.engine.test.util.JSFileUtils;

/**
 * This class is a stress test for JobScheduler. You can determine the number of tasks and the runtime of each
 * task with ESTIMATED_TASKS and JOB_RUNTIME_IN_SECONDS.
 * Please note the JobScheduler installation supports 200 maximum parallel processes at the time. To run
 * a test with more parallel processes you you to change max_processes in spooler.h or use the binaries 
 * of version 1.3.12.1346-BT.
 * 
 *  @see JS-721
 *  
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 27.01.2012 09:05:52
 *
 */
public class StressTest extends SchedulerTest implements TaskInfoListener {

    private static final Logger logger = Logger.getLogger(StressTest.class);
    private final JSCommandBuilder util = new JSCommandBuilder();

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
        controller().activateScheduler("-e -log-level=debug","-log=" + JSFileUtils.getLocalFile(this.getClass(), "scheduler.log"));
//        controller().activateScheduler();
        File resultFile = JSFileUtils.getLocalFile(this.getClass(), "result.csv");
        JMSTaskObserver l = JMSTaskObserver.getInstance(providerUrl,ESTIMATED_TASKS, resultFile);
        l.addListener(this);
        l.start(1000L,1000L);
		for (int i=0; i < ESTIMATED_TASKS; i++) {
			controller().scheduler().executeXml(
					util.startJobImmediately(jobName)
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

	@Override
	public void onInterval(TaskInfo info) {
		logger.info(info.currentlyRunningTasks() + " tasks running");
	}
    
}
