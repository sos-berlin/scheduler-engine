package com.sos.scheduler.engine.plugins.event;

import org.apache.log4j.pattern.LogEvent;

import com.sos.scheduler.engine.kernel.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.kernel.order.ModifiableOrderEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderResumedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.order.OrderSuspendedEvent;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;

import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.*;

public class JMSEventAdapter {
	
	private static String eventName;	//FIXME Statische veränderliche Variable, das kann böse enden. Zschimmer 

    // @Deprecated // Nicht übersetzbar, Zschimmer 2011-06-07
	public static JSEvent createEvent(final SchedulerObjectFactory objFactory, final Event event) throws SchedulerException {

		eventName = event.getClass().getName().replace(event.getClass().getPackage().getName() + ".", "");
		boolean flgFound = false;
		JSEvent ev = null;
		if (event instanceof ModifiableOrderEvent) {

			ModifiableOrderEvent kernelEvent = (ModifiableOrderEvent)event;

//            throw new IllegalStateException("Nicht übersetzbarer Code, Zschimmer 2011-06-07, " + JMSEventAdapter.class.getClass());
			if (event instanceof OrderTouchedEvent) {
				ev = objFactory.createEvent("EventOrderTouched");
				EventOrderTouched oe = ev.getEventOrderTouched();
				oe.setOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}

			if (event instanceof OrderStateChangedEvent) {
				ev = objFactory.createEvent("EventOrderStateChanged");
				EventOrderStateChanged oe = ev.getEventOrderStateChanged();
				oe.setOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				OrderStateChangedEvent orderStateEvent = (OrderStateChangedEvent)event;
				oe.setPreviousState( orderStateEvent.getPreviousState().toString() );
				flgFound = true;
			}

			if (event instanceof OrderFinishedEvent) {
				ev = objFactory.createEvent("EventOrderFinished");
				EventOrderFinished oe = ev.getEventOrderFinished();
				oe.setOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}

			if (event instanceof OrderSuspendedEvent) {
				ev = objFactory.createEvent("EventOrderSuspended");
				EventOrderSuspended oe = ev.getEventOrderSuspended();
				oe.setOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}

			if (event instanceof OrderResumedEvent) {
				ev = objFactory.createEvent("EventOrderResumed");
				EventOrderResumed oe = ev.getEventOrderResumed();
				oe.setOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}
		}

//      FIXME Auskommentiert, weil SchedulerCloseEvent.getObject() nicht mehr zugänglich ist. Brauchen wir das denn hier?		
//		if (event instanceof SchedulerCloseEvent) {
//			ev = objFactory.createEvent("EventSchedulerClosed");
//			EventSchedulerClosed oe = ev.getEventSchedulerClosed();
//			SchedulerCloseEvent closeEvent = (SchedulerCloseEvent)event;
//			//FIXME Brauchen wir das? Zschimmer oe.setScheduler( JMSSchedulerAdapter.createInstance(closeEvent.getObject()));
//			flgFound = true;
//		}
			
		if (event instanceof ErrorLogEvent) {
			ev = objFactory.createEvent("EventLogError");
			EventLogError oe = ev.getEventLogError();
			ErrorLogEvent logEvent = (ErrorLogEvent)event;
			oe.setLog( JMSMessageAdapter.createInstance(logEvent.getMessage()) );
			flgFound = true;
		}
			
		
		if (!flgFound) {
			throw new SchedulerException("the event " + event.getClass().getName() + " is not known in class " + JMSEventAdapter.class.getName() + " - change the constructor of the class.");
		}
		
		return ev;

	}
	
}
