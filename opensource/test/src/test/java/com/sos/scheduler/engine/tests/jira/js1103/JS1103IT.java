package com.sos.scheduler.engine.tests.jira.js1103;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;
import java.io.IOException;


public class JS1103IT extends SchedulerTest {

    @Test
    public void test() throws IOException {
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }

    @EventHandler
    public void handleOrderEnd(OrderFinishedEvent e) {
        controller().terminateScheduler();
    }
}
