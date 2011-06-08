package com.sos.scheduler.engine.plugins.guicommand;

import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import org.junit.Ignore;
import org.junit.Test;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public class MySchedulerTest extends SchedulerTest {
    public MySchedulerTest() {
        startScheduler("-e");
    }


    @Test public void test1() throws Exception {
        String result = executeGuiCommand("<test/>");
        assertThat(result, containsString("<testResult"));
    }

    
    @Test public void testShowTaskHistory() throws Exception {
        String result = executeGuiCommand("<showTaskHistory/>");
        assertThat(result, containsString("</myResult>"));
        assertThat(result, containsString("<row "));
        assertThat(result, containsString(" clusterMemberId="));
    }


    private String executeGuiCommand(String guiXml) throws Exception {
        String commandXml =
                "<plugin.command plugin_class='" + GUICommandPlugin.class.getName() + "'>" +
                guiXml +
                "</plugin.command>";
        return getScheduler().executeXml(commandXml);
    }
}
