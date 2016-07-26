package com.sos.scheduler.engine.tests.jira.js1026;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import java.io.IOException;
import org.junit.Test;
import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.MatcherAssert.assertThat;
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
        controller().waitForTermination();
    }

    @HotEventHandler
    public void handleOrderEnd(OrderFinishedEvent e, UnmodifiableOrder order) throws IOException, InterruptedException {
        scala.collection.Map<String,String> map = order.variables();
        assertThat(map.apply("testvar1"), equalTo("value1"));
        assertThat(map.apply("testvar2"), equalTo("newvalue2"));
        assertThat(map.apply("testvar3"), equalTo("value3"));
        assertThat(map.apply("testvar4"), equalTo("newvalue4"));
        assertThat(map.apply("testvar5"), equalTo("value5"));
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
