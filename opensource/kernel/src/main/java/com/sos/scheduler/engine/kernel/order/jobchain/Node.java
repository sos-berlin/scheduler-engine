package com.sos.scheduler.engine.kernel.order.jobchain;

import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.NodeC;
import com.sos.scheduler.engine.kernel.cppproxy.NodeCI;
import com.sos.scheduler.engine.kernel.cppproxy.Order_queue_nodeC;
import com.sos.scheduler.engine.kernel.order.OrderState;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;

/**
 *
 * @author Zschimmer.sos
 */
public class Node extends AbstractHasPlatform implements Sister {
    private final NodeCI nodeC;


    Node(Platform platform, NodeCI nodeC) {
        super(platform);
        this.nodeC = nodeC;
    }


    @Override public final void onCppProxyInvalidated() {}


    public final OrderState getOrderState() { return OrderState.of(nodeC.string_order_state()); }
    public final OrderState getNextState() { return OrderState.of(nodeC.string_next_state()); }
    public final OrderState getErrorState() { return OrderState.of(nodeC.string_error_state()); }


    public static class Type implements SisterType<Node, NodeC> {
        @Override public Node sister(NodeC proxy, Sister context) {
            NodeCI nodeCI = proxy;
            return nodeCI instanceof Order_queue_nodeC?
                Order_queue_nodeC.sisterType.sister((Order_queue_nodeC)nodeCI, context)
              : new Node(Platform.of(context), proxy);
        }
    }
}
