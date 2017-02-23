package com.sos.scheduler.engine.tests.jira.js948;

import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.job.JobPath;
import com.sos.scheduler.engine.data.job.TaskEnded;
import com.sos.scheduler.engine.data.job.TaskKey;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;
import static org.junit.Assert.assertEquals;

/**
 * @author ss
 * at 15.05.13 08:33
 */
public class JS948IT extends SchedulerTest {

    private static final JobPath repeatJobPath = new JobPath("/repeat");
    private static final JobPath absoluteRepeatJobPath = new JobPath("/absolute-repeat");

    private int maxModifyCommands;
    private int maxTasks;
    private JobPath jobPath;
    private int tasksCompleted;
    private int modifyCommands;

    @Test
    public void singleWakeUpRepeat() {
        init(1, repeatJobPath);
        controller().activateScheduler();
        controller().waitForTermination();
    }

    @Test
    public void multipleWakeUpRepeat() {
        init(2, repeatJobPath);
        controller().activateScheduler();
        controller().waitForTermination();
    }

    @Test
    public void singleWakeUpAbsoluteRepeat() {
        init(1, absoluteRepeatJobPath);
        controller().activateScheduler();
        controller().waitForTermination();
    }

    @Test
    public void multipleWakeUpAbsoluteRepeat() {
        init(2, absoluteRepeatJobPath);
        controller().activateScheduler();
        controller().waitForTermination();
    }

    private void init(int maxModifyCommands, JobPath jobPath) {
        this.maxModifyCommands = maxModifyCommands;
        this.maxTasks = maxModifyCommands + 1;
        this.jobPath = jobPath;
        this.tasksCompleted = 0;
        this.modifyCommands = 0;
    }

    @EventHandler
    public void handleEvent(KeyedEvent<TaskEnded> keyedEvent) {
        TaskKey taskKey = (TaskKey)keyedEvent.key();
        tasksCompleted++;
        if (tasksCompleted == 1)
            assertEquals("repeat", taskKey.jobPath().name());       // läuft beim Start des JobScheduler automatisch an

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
