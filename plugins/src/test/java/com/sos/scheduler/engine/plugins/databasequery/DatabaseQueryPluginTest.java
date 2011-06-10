package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import org.junit.Test;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public class DatabaseQueryPluginTest extends SchedulerTest {
    public DatabaseQueryPluginTest() throws Exception {
        startScheduler();
        waitForTaskTermination();
    }


    private void waitForTaskTermination() throws Exception {
        Thread.sleep(3*1000);   // TODO Besser TerminatedTaskEvent abwarten, daber das haben wir noch nicht.
    }


    @Test public void testShowTaskHistory() throws Exception {
        String result = execute("<showTaskHistory/>");
        assertThat(result, containsString("</myResult>"));
        assertThat(result, containsString("<row "));
        assertThat(result, containsString(" clusterMemberId="));
    }


    private String execute(String subcommandXml) throws Exception {
        String commandXml =
                "<plugin.command plugin_class='" + DatabaseQueryPlugin.class.getName() + "'>" +
                subcommandXml +
                "</plugin.command>";
        return getScheduler().executeXml(commandXml);
    }
}
