package com.sos.scheduler.engine.tests.jira.js731;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;
import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

/** @see <a href='http://www.sos-berlin.com/jira/browse/JS-731'>JS-731</a> */
public final class JS731IT extends SchedulerTest {
    @Test public void testOrderParametersNamesAndGet() {
        controller().activateScheduler();
        String params = "<params><param name='a' value='ä'/><param name='B' value='B'/></params>";
        scheduler().executeXml("<add_order job_chain='a' id='1'>" + params + "</add_order>");
        controller().waitForTermination();
    }

    @HotEventHandler public void handleEvent(OrderFinishedEvent e, UnmodifiableOrder order) {
        scala.collection.Map<String, String> v = order.variables();
        assertThat(v.apply("a"), equalTo("ä"));
        assertThat(v.apply("B"), equalTo("B"));
        controller().terminateScheduler();
    }
}
