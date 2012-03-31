package com.sos.scheduler.engine.tests.jira.js653;

import com.google.common.base.Objects;
import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.data.order.OrderState;

class OrderIdAndState {
    private final OrderId orderId;
    private final OrderState orderState;

    OrderIdAndState(OrderId orderId, OrderState orderState) {
        this.orderId = orderId;
        this.orderState = orderState;
    }

    @Override public int hashCode() {
        return Objects.hashCode(orderId, orderState);
    }

    @Override public boolean equals(Object other) {
        return other instanceof OrderIdAndState && eq((OrderIdAndState)other);
    }

    private boolean eq(OrderIdAndState o) {
        return orderId.equals(o.orderId) && orderState.equals(o.orderState);
    }

    @Override public String toString() {
        return orderId + "(" + orderState + ")";
    }

    static OrderIdAndState of(String orderId, String orderState) {
        return new OrderIdAndState(new OrderId(orderId), new OrderState(orderState));
    }
}
