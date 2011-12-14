package com.sos.scheduler.engine.tests.scheduler.scheduler;

import com.sos.scheduler.engine.kernel.util.XmlUtils;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

public class SchedulerSelfTest extends SchedulerTest {
    @Test public void testExecuteXml() {
        controller().activateScheduler();
        XmlUtils.loadXml(scheduler().executeXml("<show_state/>"));
    }

    @Test(expected=Exception.class) public void testFailingExecuteXml() {
        controller().activateScheduler();
        scheduler().executeXml("<UNKNOWN/>");
    }
}
