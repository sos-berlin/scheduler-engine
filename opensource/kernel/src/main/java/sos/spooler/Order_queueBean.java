package sos.spooler;

import static sos.spooler.Beans.toDelegate;

public final class Order_queueBean implements Bean<Order_queue> {
    private final Order_queue delegate;

    public Order_queueBean(Order_queue delegate) {
        this.delegate = delegate;
    }

    public int getLength() {
        return delegate.length();
    }

    public Order add_order(OrderBean order) {
        return delegate.add_order(toDelegate(order));
    }

    @Override public Order_queue getDelegate() {
        return delegate;
    }
}
