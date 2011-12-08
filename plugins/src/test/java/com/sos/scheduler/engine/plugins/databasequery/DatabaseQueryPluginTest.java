package com.sos.scheduler.engine.plugins.databasequery;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsString;

import org.apache.log4j.Logger;
import org.junit.Test;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.Gate;
import com.sos.scheduler.engine.test.SchedulerTest;

public final class DatabaseQueryPluginTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(DatabaseQueryPluginTest.class);
    
    private Time timeout = Time.of(20);

    private static final Gate<Boolean> gate = new Gate<Boolean>();

    public DatabaseQueryPluginTest() throws Exception {
        controller().useDatabase();
        controller().startScheduler();
    }

    @Test
    public void testShowTaskHistory() throws Exception {
        waitForTaskTermination();
        String result = execute("<showTaskHistory/>");
        assertThat(result, containsString("</myResult>"));
        assertThat(result, containsString("<row "));
        assertThat(result, containsString(" job="));
        controller().terminateScheduler();
    }

    private void waitForTaskTermination() throws Exception {
    	controller().waitUntilSchedulerIsActive();
        gate.expect(true, timeout);
    }

    @EventHandler public void handleEvent(TaskEndedEvent e) throws InterruptedException {
        gate.put(true);
    }

    private String execute(String subcommandXml) throws Exception {
        String commandXml =
                "<plugin.command plugin_class='" + DatabaseQueryPlugin.class.getName() + "'>" +
                    subcommandXml +
                "</plugin.command>";
        String result = scheduler().executeXml(commandXml);
        logger.debug(result);
        return result;
    }
}
