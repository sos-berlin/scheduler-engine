package com.sos.scheduler.engine.tests.jira.js948;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

import com.sos.scheduler.engine.data.job.JobPath;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;

/**
 * @author ss
 * at 15.05.13 08:33
 */
public class JS948IT extends SchedulerTest {

    private static final JobPath repeatJobPath = JobPath.of("/repeat");
    private static final JobPath absoluteRepeatJobPath = JobPath.of("/absolute-repeat");

    private int maxModifyCommands;
    private int maxTasks;
    private JobPath jobPath;
    private int tasksCompleted;
    private int modifyCommands;

    @Test
    public void singleWakeUpRepeat() {
        init(1, repeatJobPath);
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }

    @Test
    public void multipleWakeUpRepeat() {
        init(2, repeatJobPath);
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }

    @Test
    public void singleWakeUpAbsoluteRepeat() {
        init(1, absoluteRepeatJobPath);
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }

    @Test
    public void multipleWakeUpAbsoluteRepeat() {
        init(2, absoluteRepeatJobPath);
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }

    private void init(int maxModifyCommands, JobPath jobPath) {
        this.maxModifyCommands = maxModifyCommands;
        this.maxTasks = maxModifyCommands + 1;
        this.jobPath = jobPath;
        this.tasksCompleted = 0;
        this.modifyCommands = 0;
    }

    @EventHandler
    public void handleEvent(TaskEndedEvent e) {
        tasksCompleted++;
        if (tasksCompleted == 1)
            assertEquals("repeat", e.jobPath().name());       // läuft beim Start des JobScheduler automatisch an

        // Erneuter Start durch wake_when_in_period
        if (modifyCommands < maxModifyCommands) {          // absolute_repeat.job wird beim Start des JobScheduler 1x ausgeführt
            modifyCommands++;
            scheduler().executeXml("<modify_job job=\"" + jobPath.string() + "\" cmd=\"wake_when_in_period\" />");
        }

        if (tasksCompleted == maxTasks) {
            controller().terminateScheduler();
        }
    }
}
