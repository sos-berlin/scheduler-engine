package com.sos.scheduler.engine.tests.jira.js653;

import com.google.common.base.Objects;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.kernel.order.OrderKey;
import com.sos.scheduler.engine.kernel.order.OrderState;

class OrderKeyAndState {
    private final OrderKey orderKey;
    private final OrderState orderState;

    OrderKeyAndState(OrderKey orderKey, OrderState orderState) {
        this.orderKey = orderKey;
        this.orderState = orderState;
    }

    @Override public int hashCode() {
        return Objects.hashCode(orderKey, orderState);
    }

    @Override public boolean equals(Object other) {
        return other instanceof OrderKeyAndState && eq((OrderKeyAndState)other);
    }

    private boolean eq(OrderKeyAndState o) {
        return orderKey.equals(o.orderKey) && orderState.equals(o.orderState);
    }

    @Override public String toString() {
        return orderKey + "(" + orderState + ")";
    }

    static OrderKeyAndState of(String jobChainPath, String orderId, String orderState) {
        return new OrderKeyAndState(
                new OrderKey(new AbsolutePath(jobChainPath), new OrderId(orderId)),
                new OrderState(orderState));
    }
}
