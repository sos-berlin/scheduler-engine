/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.tests.excluded.ss.kill;


import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent;
import com.sos.scheduler.engine.kernel.job.events.TaskStartedEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import com.sos.scheduler.engine.test.util.FileUtils;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.File;
import java.io.IOException;

import static org.junit.Assert.assertTrue;

/**
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 08.03.2012 08:09:22
 *
 */
public class KillTest extends SchedulerTest {
    
    private final static Logger logger = Logger.getLogger(KillTest.class);

    private final String job = "java_simple";
	private final CommandBuilder util = new CommandBuilder();
    public static final Time timeout = Time.of(120);

	@Test
	public void testKill() {
        File resultFile = FileUtils.getResourceFile(this.getClass(), "scheduler.log");
        logger.info("logfile=" + resultFile);
		controller().activateScheduler("-e","-log-level=info","-log=" + resultFile);
        wait(10);
		controller().scheduler().executeXml( util.startJobImmediately(job).getCommand() );

        logger.info("ready");
		controller().waitForTermination(timeout);
		testAssertions();
	}
    
    private void wait(int seconds) {
        try {
            Thread.sleep(seconds * 1000);
        } catch (InterruptedException e1) {
            e1.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }
    }
	
    @EventHandler
    public void handleTaskStart(TaskStartedEvent e) {
        UnmodifiableTask task = (UnmodifiableTask)e.getObject();
        String id = task.getId().getString();
        logger.info("task started with id " + id);
        wait(2);
        logger.info("killing task with id " + id);
        controller().scheduler().executeXml(util.killTaskImmediately(job, id).getCommand());
        logger.info("task with id " + id + " was killed.");
        wait(2);
    }

	public void testAssertions() {
	}

}
