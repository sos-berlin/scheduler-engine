package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.data.order.OrderKey;
import com.sos.scheduler.engine.data.order.OrderState;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobchain;

public class UnmodifiableOrderDelegate implements UnmodifiableOrder {
    private final UnmodifiableOrder order;

    public UnmodifiableOrderDelegate(UnmodifiableOrder order) {
        this.order = order;
    }

    @Override public OrderKey getKey() {
        return order.getKey();
    }

    @Override public OrderId getId() {
        return order.getId();
    }

    @Override public OrderState getState() {
        return order.getState();
    }

    @Override public OrderState getEndState() {
        return order.getEndState();
    }

    @Override public String getTitle() {
        return order.getTitle();
    }

    @Override public UnmodifiableJobchain getJobChain() {
        return order.getJobChain();
    }

    @Override public UnmodifiableJobchain getJobChainOrNull() {
        return order.getJobChainOrNull();  //TODO Delegate erzeugen
    }

    @Override public UnmodifiableVariableSet getParameters() {
        return order.getParameters();   // TODO Delegate erzeugen
    }

    @Override public PrefixLog getLog() {
        return order.getLog();
    }

    @Override public String toString() {
        return order.toString();
    }
}
