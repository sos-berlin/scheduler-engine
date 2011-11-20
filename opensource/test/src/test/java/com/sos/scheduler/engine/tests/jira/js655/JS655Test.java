package com.sos.scheduler.engine.tests.jira.js655;

import org.junit.Ignore;
import org.junit.Test;

import com.sos.scheduler.engine.test.SchedulerTest;

public class JS655Test extends SchedulerTest {
    @Ignore // SCHEDULER-161  There is no Job_chain '/myLazyJobChain'
    @Test public void test() {
        controller().startScheduler("-e");
        controller().waitUntilSchedulerIsActive();
        controller().terminateScheduler();
    }
}
