package com.sos.scheduler.engine.tests.excluded.ss.runtime;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.data.order.OrderStepStartedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public final class RuntimeTest extends SchedulerTest {

	private static final int maxOrderCount = 2;
	private static int numberOfStartedOrders = 0;
    private int finishedOrderCount = 0;

    @Test
    // Started die Aufträge sofort, sie laufen dann auch sofort an.
    public void testImmediately() throws Exception {
        numberOfStartedOrders = 0;
        controller().activateScheduler();
        addOrders(maxOrderCount, "now");
        controller().waitForTermination(shortTimeout);
    }

    @Test
    // Started die Aufträge zeitverzögert, der zweite Auftrag läuft erst dann an, wenn der erste beendet worden ist.
    public void testScheduled() throws Exception {
        numberOfStartedOrders = 0;
        controller().activateScheduler();
        addOrders(maxOrderCount, "now+1");
        controller().waitForTermination(shortTimeout);
    }

    private void addOrders(int n, String startAt) {
        for (int i = 0; i < n; i++) {
            scheduler().executeXml("<add_order job_chain='/A' id='" + i + "' at='" + startAt + "' />");
        }
    }

    @EventHandler
    public void handle(OrderStepStartedEvent e) {
        numberOfStartedOrders++;
    }

    @EventHandler
    public void handle(OrderFinishedEvent e) {

        // alle Aufträge müssen bereits gestartet worden sein, wenn ein Auftrag beendet wird.
        assertEquals(maxOrderCount,numberOfStartedOrders);

        finishedOrderCount += 1;
        if (finishedOrderCount == maxOrderCount )
            controller().terminateScheduler();

    }
}
