package com.sos.scheduler.engine.kernel.order.jobchain;

import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.NodeCI;

public class JobNode extends Node {
    JobNode(Platform platform, NodeCI nodeC) {
        super(platform, nodeC);
    }
}
