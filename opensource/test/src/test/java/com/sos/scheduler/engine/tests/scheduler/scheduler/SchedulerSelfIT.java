package com.sos.scheduler.engine.tests.scheduler.scheduler;

import com.sos.scheduler.engine.common.xml.XmlUtils;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

public final class SchedulerSelfIT extends SchedulerTest {
    @Test public void testExecuteXml() {
        controller().activateScheduler();
        XmlUtils.loadXml(scheduler().executeXml("<show_state/>"));
    }

    @Test(expected=Exception.class) public void testFailingExecuteXml() {
        controller().activateScheduler();
        scheduler().executeXml("<UNKNOWN/>");
    }
}
