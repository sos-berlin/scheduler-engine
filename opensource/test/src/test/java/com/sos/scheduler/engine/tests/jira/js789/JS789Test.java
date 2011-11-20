package com.sos.scheduler.engine.tests.jira.js789;

import org.junit.Test;

import com.sos.scheduler.engine.main.SchedulerState;
import com.sos.scheduler.engine.test.SchedulerTest;

/** @see <a href="http://www.sos-berlin.com/jira/browse/JS-789">JS-789</a> */
public final class JS789Test extends SchedulerTest {
    @Test public void test() throws Exception {
        controller().startScheduler();
        controller().waitUntilSchedulerState(SchedulerState.active);
        controller().terminateScheduler();
    }
}
