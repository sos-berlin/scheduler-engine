package com.sos.scheduler.engine.plugins.guicommand;

import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import org.junit.Ignore;
import org.junit.Test;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public class MySchedulerTest1 extends MySchedulerTest {
    @Test public void test1() throws Exception {
        String result = executeGuiCommand("<test/>");
        assertThat(result, containsString("<testResult"));
    }
}
