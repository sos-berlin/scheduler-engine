package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.kernel.order.OrderEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;

import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.*;

public class JMSEventAdapter {
	
	private static String eventName;
	
	public static JSEvent createEvent(final SchedulerObjectFactory objFactory, final Event event) {

		eventName = event.getClass().getName().replace(event.getClass().getPackage().getName() + ".", "");
		boolean flgFound = false;
		JSEvent ev = null;
		if (event instanceof OrderEvent) {

			OrderEvent kernelEvent = (OrderEvent)event;
			
			if (event instanceof OrderTouchedEvent) {
				ev = objFactory.createEvent("EventOrderTouched");
				EventOrderTouched oe = ev.getEventOrderTouched();
				oe.setOrder( JMSOrderAdapter.createEventOrder(kernelEvent.getOrder()) );
				flgFound = true;
			}
		
			if (event instanceof OrderStateChangedEvent) {
				ev = objFactory.createEvent("EventOrderStateChanged");
				EventOrderStateChanged oe = ev.getEventOrderStateChanged();
				oe.setOrder( JMSOrderAdapter.createEventOrder(kernelEvent.getOrder()) );
				OrderStateChangedEvent orderStateEvent = (OrderStateChangedEvent)event;
				oe.setPreviousState( orderStateEvent.getPreviousState().toString() );
				flgFound = true;
			}
			
			if (event instanceof OrderFinishedEvent) {
				ev = objFactory.createEvent("EventOrderFinished");
				EventOrderFinished oe = ev.getEventOrderFinished();
				oe.setOrder( JMSOrderAdapter.createEventOrder(kernelEvent.getOrder()) );
				flgFound = true;
			}
			
		}
		
		if (!flgFound) {
			throw new SchedulerException("the event " + eventName + " is not known in class " + JMSEventAdapter.class.getName() + " - change the constructor of the class.");
		}
		
		return ev;

	}
	
}
