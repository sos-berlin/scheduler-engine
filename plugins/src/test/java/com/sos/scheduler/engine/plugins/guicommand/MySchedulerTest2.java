package com.sos.scheduler.engine.plugins.guicommand;

import org.junit.Test;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public class MySchedulerTest2 extends MySchedulerTest {
    //@Ignore //TODO Wir brauchen noch die Datenbank
    @Test public void testShowTaskHistory() throws Exception {
        String result = executeGuiCommand("<showTaskHistory/>");
        assertThat(result, containsString("</myResult>"));
        assertThat(result, containsString("<row "));
        assertThat(result, containsString(" clusterMemberId="));
    }
}
