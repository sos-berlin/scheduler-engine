package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.cppproxy.OrderCMock;
import org.junit.Test;

public final class OrderTest {
    private final UnmodifiableOrder order = new Order(new OrderCMock());

    @Test public void testDummy() {
        order.getId();
    }
}
