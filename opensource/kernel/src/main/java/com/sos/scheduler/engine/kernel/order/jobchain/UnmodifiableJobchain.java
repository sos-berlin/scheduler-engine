package com.sos.scheduler.engine.kernel.order.jobchain;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.data.folder.AbsolutePath;
import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;

public interface UnmodifiableJobchain {
    String getName();
    AbsolutePath getPath();
    ImmutableList<Node> getNodes();
    UnmodifiableOrder order(OrderId id);
}