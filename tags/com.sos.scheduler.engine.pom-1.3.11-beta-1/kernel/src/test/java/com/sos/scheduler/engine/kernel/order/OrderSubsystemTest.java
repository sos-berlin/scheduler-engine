package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.PlatformMock;
import com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemC;
import com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemCMock;
import org.junit.*;


public class OrderSubsystemTest
{
    private final Platform platform = PlatformMock.newInstance();

    private final Order_subsystemC order_subsystemC = new Order_subsystemCMock();

    @Test public void testDummy() {}
}
