package com.sos.scheduler.engine.plugins.databasequery;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsString;

import org.junit.Test;

import com.sos.scheduler.engine.kernel.test.SchedulerTest;

public final class DatabaseQueryPluginTest extends SchedulerTest {
    public DatabaseQueryPluginTest() throws Exception {
        controller().startScheduler("-e");
        waitForTaskTermination();
    }

    private void waitForTaskTermination() throws Exception {
    	controller().waitUntilSchedulerIsRunning();
        Thread.sleep(10*1000);   // TODO Besser TaskTerminatedEvent abwarten, aber das haben wir noch nicht.
    }

    @Test
    public void testShowTaskHistory() throws Exception {
        String result = execute("<showTaskHistory/>");
        assertThat(result, containsString("</myResult>"));
        assertThat(result, containsString("<row "));
        assertThat(result, containsString(" job="));
    }

    private String execute(String subcommandXml) throws Exception {
        String commandXml =
                "<plugin.command plugin_class='" + DatabaseQueryPlugin.class.getName() + "'>" +
                    subcommandXml +
                "</plugin.command>";
        return scheduler().executeXml(commandXml);
    }
}
