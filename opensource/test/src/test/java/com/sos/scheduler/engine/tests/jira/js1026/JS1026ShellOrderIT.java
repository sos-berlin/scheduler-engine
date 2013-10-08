package com.sos.scheduler.engine.tests.jira.js1026;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.Test;

import java.io.IOException;

import static org.junit.Assert.assertTrue;

/**
 * This is a test for SCHEDULER_RETURN_VALUES in order shell jobs.
 * The test uses jobchain chain.
 * The order jobs test1_order and test2_order will be started.
 *
 * @author Florian Schreiber
 * @version 1.0 - 24.09.2013 13:39:41
 */
public class JS1026ShellOrderIT extends SchedulerTest {

    private static final String jobchain = "chain";
    private final CommandBuilder util = new CommandBuilder();

    @Test
    public void test() throws IOException {
        controller().activateScheduler();
        controller().scheduler().executeXml(util.addOrder(jobchain).getCommand());
        controller().waitForTermination(shortTimeout);
    }

    @HotEventHandler
    public void handleOrderEnd(OrderFinishedEvent e, UnmodifiableOrder order) throws IOException, InterruptedException {
        ImmutableMap<String,String> map = order.getParameters().toGuavaMap();
        assertObject(map, "testvar1", "value1");
        assertObject(map, "testvar2", "newvalue2");
        assertObject(map, "testvar3", "value3");
        assertObject(map, "testvar4", "newvalue4");
        assertObject(map, "testvar5", "value5");
        controller().terminateScheduler();
    }

    /**
     * checks if an estimated object was given
     */
    private static void assertObject(ImmutableMap<String, String> map, String key, String expectedValue) {
        assertTrue(key + " is no valid key", map.containsKey(key));
        String val = map.get(key);
        assertTrue(val + " is no valid value - " + expectedValue + " expected", val.equals(expectedValue));
    }
}