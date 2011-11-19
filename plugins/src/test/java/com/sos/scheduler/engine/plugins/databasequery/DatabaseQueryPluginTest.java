package com.sos.scheduler.engine.plugins.databasequery;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsString;

import org.apache.log4j.Logger;
import org.junit.Test;

import com.sos.scheduler.engine.test.SchedulerTest;

public final class DatabaseQueryPluginTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(DatabaseQueryPluginTest.class);

    public DatabaseQueryPluginTest() throws Exception {
        controller().setSettings(temporaryDatabaseSettings());
        controller().startScheduler();
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
        String result = scheduler().executeXml(commandXml);
        logger.debug(result);
        return result;
    }
}
