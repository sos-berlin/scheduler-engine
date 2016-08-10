package com.sos.scheduler.engine.tests.jira.js653;

import com.google.common.base.Objects;
import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.data.jobchain.NodeId;

class OrderIdAndNodeId {
    private final OrderId orderId;
    private final NodeId nodeId;

    OrderIdAndNodeId(OrderId orderId, NodeId nodeId) {
        this.orderId = orderId;
        this.nodeId = nodeId;
    }

    @Override public int hashCode() {
        return Objects.hashCode(orderId, nodeId);
    }

    @Override public boolean equals(Object other) {
        return other instanceof OrderIdAndNodeId && eq((OrderIdAndNodeId)other);
    }

    private boolean eq(OrderIdAndNodeId o) {
        return orderId.equals(o.orderId) && nodeId.equals(o.nodeId);
    }

    @Override public String toString() {
        return orderId + "(" + nodeId + ")";
    }

    static OrderIdAndNodeId of(String orderId, String nodeId) {
        return new OrderIdAndNodeId(new OrderId(orderId), new NodeId(nodeId));
    }
}
