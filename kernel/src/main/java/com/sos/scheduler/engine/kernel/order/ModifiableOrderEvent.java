package com.sos.scheduler.engine.kernel.order;

public class ModifiableOrderEvent extends OrderEvent {
	private final Order order;
	
    public ModifiableOrderEvent(Order o) {
        super(o);
        this.order = o;
    }

    public final Order getOrder() {
        return order;
    }
}
