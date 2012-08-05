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

    @Override public final OrderKey getKey() {
        return order.getKey();
    }

    @Override public final OrderId getId() {
        return order.getId();
    }

    @Override public final OrderState getState() {
        return order.getState();
    }

    @Override public final OrderState getEndState() {
        return order.getEndState();
    }

    @Override public final String getTitle() {
        return order.getTitle();
    }

    @Override public final UnmodifiableJobchain getJobChain() {
        return order.getJobChain();
    }

    @Override public final UnmodifiableJobchain getJobChainOrNull() {
        return order.getJobChainOrNull();  //TODO Delegate erzeugen
    }

    @Override public final UnmodifiableVariableSet getParameters() {
        return order.getParameters();   // TODO Delegate erzeugen
    }

    @Override public final PrefixLog getLog() {
        return order.getLog();
    }

    @Override public String toString() {
        return order.toString();
    }
}
