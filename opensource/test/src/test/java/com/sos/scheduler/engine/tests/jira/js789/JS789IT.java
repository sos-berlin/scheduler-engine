package com.sos.scheduler.engine.tests.jira.js789;

import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

/** @see <a href="http://www.sos-berlin.com/jira/browse/JS-789">JS-789</a> */
public final class JS789IT extends SchedulerTest {
    @Test public void test() throws Exception {
        controller().activateScheduler();
        controller().terminateScheduler();
    }
}
