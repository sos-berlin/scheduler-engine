package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.UnmodifiableVariableSet;
import com.sos.scheduler.engine.kernel.log.SchedulerLogger;
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobchain;

public class UnmodifiableOrderDelegate implements UnmodifiableOrder {
    private final UnmodifiableOrder order;

    public UnmodifiableOrderDelegate(UnmodifiableOrder order) {
        this.order = order;
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

    @Override public UnmodifiableJobchain jobchainOrNull() {
        return order.jobchainOrNull();  //TODO Delegate erzeugen
    }

    @Override public UnmodifiableVariableSet getParameters() {
        return order.getParameters();   // TODO Delegate erzeugen
    }

    @Override public Platform getPlatform() {
        return order.getPlatform();
    }

    @Override public SchedulerLogger log() {
        return order.log();
    }
}
