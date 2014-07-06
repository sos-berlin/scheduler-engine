package com.sos.scheduler.engine.tests.jira.js1104;

import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

import java.io.IOException;


public class JS1104IT extends SchedulerTest {

    @Test
    public void test() throws IOException {
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }
}
