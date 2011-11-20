package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.scheduler.PlatformMock;
import com.sos.scheduler.engine.kernel.cppproxy.*;
import org.junit.Test;


public class OrderTest
{
    private final Platform platform = PlatformMock.newInstance();
    private final OrderC orderC = new OrderCMock();
    private final UnmodifiableOrder order = new Order(platform, orderC);

    @Test public void testDummy() {
        order.getId();
    }
}
