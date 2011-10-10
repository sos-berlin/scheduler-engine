package com.sos.scheduler.engine.plugins.event;

import com.sos.scheduler.engine.kernel.order.OrderEvent;
import com.sos.scheduler.engine.kernel.schedulerevent.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.order.ModifiableOrderEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderResumedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStepEndedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStepStartedEvent;
import com.sos.scheduler.engine.kernel.order.OrderSuspendedEvent;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.EventLogError;
import com.sos.scheduler.model.events.EventOrderFinished;
import com.sos.scheduler.model.events.EventOrderResumed;
import com.sos.scheduler.model.events.EventOrderStateChanged;
import com.sos.scheduler.model.events.EventOrderStepEnded;
import com.sos.scheduler.model.events.EventOrderStepStarted;
import com.sos.scheduler.model.events.EventOrderSuspended;
import com.sos.scheduler.model.events.EventOrderTouched;
import com.sos.scheduler.model.events.JSEvent;

public class JMSEventAdapter {
	
	public static JSEvent createEvent(final SchedulerObjectFactory objFactory, final Event event) throws SchedulerException {

		boolean flgFound = false;
		JSEvent ev = null;
		if (event instanceof ModifiableOrderEvent) {

			ModifiableOrderEvent kernelEvent = (ModifiableOrderEvent)event;

			if (event instanceof OrderTouchedEvent) {
				ev = objFactory.createEvent("EventOrderTouched");
				EventOrderTouched oe = ev.getEventOrderTouched();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}

			if (event instanceof OrderStepStartedEvent) {
				ev = objFactory.createEvent("EventOrderStepStarted");
				EventOrderStepStarted oe = ev.getEventOrderStepStarted();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}

		}

		if (event instanceof OrderEvent) {

			OrderEvent kernelEvent = (OrderEvent)event;

			if (event instanceof OrderStateChangedEvent) {
				ev = objFactory.createEvent("EventOrderStateChanged");
				EventOrderStateChanged oe = ev.getEventOrderStateChanged();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				OrderStateChangedEvent orderStateEvent = (OrderStateChangedEvent)event;
				oe.setPreviousState( orderStateEvent.getPreviousState().toString() );
				flgFound = true;
			}

			if (event instanceof OrderFinishedEvent) {
				ev = objFactory.createEvent("EventOrderFinished");
				EventOrderFinished oe = ev.getEventOrderFinished();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}

			if (event instanceof OrderSuspendedEvent) {
				ev = objFactory.createEvent("EventOrderSuspended");
				EventOrderSuspended oe = ev.getEventOrderSuspended();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}

			if (event instanceof OrderResumedEvent) {
				ev = objFactory.createEvent("EventOrderResumed");
				EventOrderResumed oe = ev.getEventOrderResumed();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}

			if (event instanceof OrderStepEndedEvent) {
				ev = objFactory.createEvent("EventOrderStepEnded");
				EventOrderStepEnded oe = ev.getEventOrderStepEnded();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(kernelEvent.getOrder()) );
				flgFound = true;
			}

		}
		
		if (event instanceof SchedulerCloseEvent) {
			ev = objFactory.createEvent("EventSchedulerClosed");
			flgFound = true;
		}
			
		if (event instanceof ErrorLogEvent) {
			ev = objFactory.createEvent("EventLogError");
			EventLogError oe = ev.getEventLogError();
			ErrorLogEvent logEvent = (ErrorLogEvent)event;
			oe.setInfoLog( JMSMessageAdapter.createInstance(logEvent.getMessage()) );
			flgFound = true;
		}
			
		
		if (!flgFound) {
			throw new SchedulerException("the event " + event.getClass().getName() + " is not known in class " + JMSEventAdapter.class.getName() + " - change the constructor of the class.");
		}
		
		return ev;

	}
	
}
