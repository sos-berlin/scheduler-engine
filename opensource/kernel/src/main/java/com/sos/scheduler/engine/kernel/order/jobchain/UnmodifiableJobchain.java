package com.sos.scheduler.engine.kernel.order.jobchain;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;

public interface UnmodifiableJobchain {
    String getName();
    AbsolutePath getPath();
    ImmutableList<Node> getNodes();
    UnmodifiableOrder order(OrderId id);
}