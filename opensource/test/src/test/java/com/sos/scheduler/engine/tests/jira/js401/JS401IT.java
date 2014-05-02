package com.sos.scheduler.engine.tests.jira.js401;

import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.Test;
import java.io.IOException;


public class JS401IT extends SchedulerTest {

    private final CommandBuilder util = new CommandBuilder();
    private int countEndedTasks = 0;

    @Test
    public void test() throws IOException {
        controller().activateScheduler();

        controller().scheduler().executeXml("<add_order job_chain='chain1' id='1'/>");
        controller().scheduler().executeXml(util.startJobImmediately("job2").getCommand());
        controller().scheduler().executeXml(util.startJobImmediately("inc_max_non_exclusive").getCommand());

        controller().waitForTermination(shortTimeout);
    }

    @EventHandler
    public void handleEvent(TaskEndedEvent e) {
        if ( ++countEndedTasks == 3) {
            controller().terminateScheduler();
        }
    }
}
