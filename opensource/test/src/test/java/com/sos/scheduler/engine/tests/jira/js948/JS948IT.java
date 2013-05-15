package com.sos.scheduler.engine.tests.jira.js948;

import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Ignore;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static org.junit.Assert.assertEquals;

/**
 * @uthor ss
 * at 15.05.13 08:33
 */
public class JS948IT extends SchedulerTest {

    private final Logger logger = LoggerFactory.getLogger(JS948IT.class);

    private int tasksCompleted = 0;
    private int modifyCommands = 0;
    private int maxModifyCommands = 0;
    private String testJob = "";
    private int maxTasks = 0;

    @Ignore
    public void singleWakeUpRepeat() {
        init(1,"repeat");
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }

    @Ignore
    public void multipleWakeUpRepeat() {
        init(2,"repeat");
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }

    @Test
    public void singleWakeUpAbsoluteRepeat() {
        init(1,"absolute-repeat");
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }

    @Ignore
    public void multipleWakeUpAbsoluteRepeat() {
        init(2,"absolute-repeat");
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }

    private void init(int maxModifyCommands, String jobName) {
        this.maxModifyCommands = maxModifyCommands;
        this.testJob = jobName;
        this.tasksCompleted = 0;
        this.modifyCommands = 0;
        this.maxTasks = maxModifyCommands + 1;
    }

    @EventHandler
    public void handleEvent(TaskEndedEvent e) {
        tasksCompleted++;
        if (tasksCompleted == 1)
            assertEquals("repeat", e.jobPath().getName());       // läuft beim Start des JobScheduler automatisch an

        // Erneuter Start durch wake_when_in_period
        if (modifyCommands < maxModifyCommands) {          // absolute_repeat.job wird beim Start des JobScheduler 1x ausgeführt
            modifyCommands++;
            scheduler().executeXml("<modify_job job=\"/" + testJob + "\" cmd=\"wake_when_in_period\" />");
        }

        if(tasksCompleted == maxTasks) {
            controller().terminateScheduler();
        }
    }
}
