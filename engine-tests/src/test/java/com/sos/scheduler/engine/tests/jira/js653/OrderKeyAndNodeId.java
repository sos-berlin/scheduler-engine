package com.sos.scheduler.engine.tests.jira.js653;

import com.google.common.base.Objects;
import com.sos.scheduler.engine.data.jobchain.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.data.order.OrderKey;
import com.sos.scheduler.engine.data.jobchain.NodeId;

class OrderKeyAndNodeId {
    private final OrderKey orderKey;
    private final NodeId nodeId;

    OrderKeyAndNodeId(OrderKey orderKey, NodeId nodeId) {
        this.orderKey = orderKey;
        this.nodeId = nodeId;
    }

    @Override public int hashCode() {
        return Objects.hashCode(orderKey, nodeId);
    }

    @Override public boolean equals(Object other) {
        return other instanceof OrderKeyAndNodeId && eq((OrderKeyAndNodeId)other);
    }

    private boolean eq(OrderKeyAndNodeId o) {
        return orderKey.equals(o.orderKey) && nodeId.equals(o.nodeId);
    }

    @Override public String toString() {
        return orderKey + "(" + nodeId + ")";
    }

    static OrderKeyAndNodeId of(String jobChainPath, String orderId, String nodeId) {
        return new OrderKeyAndNodeId(
                new OrderKey(new JobChainPath(jobChainPath), new OrderId(orderId)),
                new NodeId(nodeId));
    }
}
