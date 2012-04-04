package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemC;
import com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemCMock;
import org.junit.Test;

public class OrderSubsystemTest {
    private final Order_subsystemC order_subsystemC = new Order_subsystemCMock();

    @Test public void testDummy() {}
}
