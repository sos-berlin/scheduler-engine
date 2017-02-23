package com.sos.scheduler.engine.tests.jira.js1107;

import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.job.TaskEnded;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.Test;

public final class JS1107IT extends SchedulerTest {

    private int countEndedTasks = 0;

    @Test
    public void test() {
        CommandBuilder util = new CommandBuilder();
        controller().activateScheduler();
        controller().scheduler().executeXml("<add_order job_chain='chain1' id='1'/>");
        controller().scheduler().executeXml(util.startJobImmediately("job2").getCommand());
        controller().scheduler().executeXml(util.startJobImmediately("inc_max_processes").getCommand());
        controller().waitForTermination();
    }

    @EventHandler
    public void handleEvent(KeyedEvent<TaskEnded> e) {
        countEndedTasks++;
        if (countEndedTasks == 3) {
            controller().terminateScheduler();
        }
    }
}
