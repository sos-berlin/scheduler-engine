package com.sos.scheduler.engine.tests.jira.js946.twoorders;

import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.data.order.OrderStepStartedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public final class JS946TwoSimultaneousOrdersIT extends SchedulerTest {

	private static final int maxOrderCount = 2;
	private int numberOfStartedOrders = 0;
    private int finishedOrderCount = 0;

    // Startet die Aufträge sofort, sie laufen dann auch sofort an.
    @Test
    public void testImmediately() {
        controller().activateScheduler();
        addOrders(1, "now");
        addOrders(2, "now");
        controller().waitForTermination(shortTimeout);
    }

    // Startet die Aufträge verzögert. Der zweite Auftrag soll nicht erst anlaufen, wenn der erste beendet worden ist.
    @Test
    public void testScheduledAtSameTime() {
        controller().activateScheduler();
        addOrders(1, "now+1");
        addOrders(2, "now+1");
        controller().waitForTermination(shortTimeout);
    }

    @Test
    public void testScheduledAtDifferentTimes() {
        controller().activateScheduler();
        addOrders(1, "now+1");
        addOrders(2, "now+2");
        controller().waitForTermination(shortTimeout);
    }

    private void addOrders(int id, String startAt) {
        scheduler().executeXml("<add_order job_chain='/A' id='" + id + "' at='" + startAt + "' />");
    }

    @EventHandler
    public void handle(OrderStepStartedEvent e) {
        numberOfStartedOrders++;
    }

    @EventHandler
    public void handle(OrderFinishedEvent e) {
        // Alle Aufträge müssen bereits gestartet worden sein, wenn ein Auftrag beendet wird.
        assertEquals(maxOrderCount, numberOfStartedOrders);
        finishedOrderCount += 1;
        if (finishedOrderCount == maxOrderCount )
            controller().terminateScheduler();
    }
}
