package com.sos.scheduler.engine.tests.jira.js1104;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.Test;
import java.io.IOException;
import java.util.Map;

/**
 * This is a test for development of an internal API Method.
 *
 * @author Florian Schreiber
 * @version 1.0 - 19.02.2014 13:39:41
 */
public class JS1104IT extends SchedulerTest {

    private static final String jobchain = "chain1";

    private final CommandBuilder util = new CommandBuilder();
    private Map<String,String> resultMap;

    @Test
    public void test() throws IOException {
        controller().activateScheduler();
        controller().waitForTermination(shortTimeout);
    }


    @EventHandler
    public void handleOrderEnd(OrderFinishedEvent e)  {
        controller().terminateScheduler();
    }
}
