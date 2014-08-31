package com.sos.scheduler.engine.tests.jira.js1103;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

public final class JS1103IT extends SchedulerTest {

    @Test
    public void test() {
        controller().activateScheduler();
        controller().waitForTermination();
    }

    @EventHandler
    public void handle(OrderFinishedEvent e) {
        controller().terminateScheduler();
    }
}
