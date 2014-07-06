package com.sos.scheduler.engine.tests.jira.js731;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

/** @see <a href='http://www.sos-berlin.com/jira/browse/JS-731'>JS-731</a> */
public final class JS731IT extends SchedulerTest {
    @Test public void testOrderParametersNamesAndGet() {
        controller().activateScheduler();
        String params = "<params><param name='a' value='ä'/><param name='B' value='B'/></params>";
        scheduler().executeXml("<add_order job_chain='a' id='1'>" + params + "</add_order>");
        controller().waitForTermination(shortTimeout);
    }

    @HotEventHandler public void handleEvent(OrderFinishedEvent e, UnmodifiableOrder order) {
        UnmodifiableVariableSet v = order.parameters();
        assertThat(v.apply("a"), equalTo("ä"));
        assertThat(v.apply("A"), equalTo("ä"));
        assertThat(v.apply("b"), equalTo("B"));
        assertThat(v.apply("B"), equalTo("B"));
        assertThat(v.getNames(), containsInAnyOrder("a", "B"));
        controller().terminateScheduler();
    }
}
