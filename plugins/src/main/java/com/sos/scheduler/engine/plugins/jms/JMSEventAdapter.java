package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.engine.data.log.ErrorLogEvent;
import com.sos.scheduler.engine.data.order.*;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.data.job.TaskEvent;
import com.sos.scheduler.engine.data.job.TaskStartedEvent;
import com.sos.scheduler.engine.data.scheduler.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.event.SimpleMessage;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.*;

public class JMSEventAdapter {
	
	public static JSEvent createEvent(final SchedulerObjectFactory objFactory, final Event event, EventSource eventSource)
            throws SchedulerException {

		boolean flgFound = false;
		JSEvent ev = null;
		if (event instanceof OrderEvent) {

            UnmodifiableOrder order = (UnmodifiableOrder)eventSource;

			if (event instanceof OrderTouchedEvent) {
				ev = objFactory.createEvent("EventOrderTouched");
				EventOrderTouched oe = ev.getEventOrderTouched();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(order) );
				flgFound = true;
			}

			if (event instanceof OrderStepStartedEvent) {
				ev = objFactory.createEvent("EventOrderStepStarted");
				EventOrderStepStarted oe = ev.getEventOrderStepStarted();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(order) );
				flgFound = true;
			}

			if (event instanceof OrderStateChangedEvent) {
				ev = objFactory.createEvent("EventOrderStateChanged");
				EventOrderStateChanged oe = ev.getEventOrderStateChanged();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(order) );
				OrderStateChangedEvent orderStateEvent = (OrderStateChangedEvent)event;
				oe.setPreviousState( orderStateEvent.getPreviousState().toString() );
				flgFound = true;
			}

			if (event instanceof OrderFinishedEvent) {
				ev = objFactory.createEvent("EventOrderFinished");
				EventOrderFinished oe = ev.getEventOrderFinished();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(order) );
				flgFound = true;
			}

			if (event instanceof OrderSuspendedEvent) {
				ev = objFactory.createEvent("EventOrderSuspended");
				EventOrderSuspended oe = ev.getEventOrderSuspended();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(order) );
				flgFound = true;
			}

			if (event instanceof OrderResumedEvent) {
				ev = objFactory.createEvent("EventOrderResumed");
				EventOrderResumed oe = ev.getEventOrderResumed();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(order) );
				flgFound = true;
			}

			if (event instanceof OrderStepEndedEvent) {
				ev = objFactory.createEvent("EventOrderStepEnded");
				EventOrderStepEnded oe = ev.getEventOrderStepEnded();
				oe.setInfoOrder( JMSOrderAdapter.createInstance(order) );
				flgFound = true;
			}

		}
		
		if (event instanceof TaskEvent) {

            UnmodifiableTask task = (UnmodifiableTask)eventSource;

			if (event instanceof TaskStartedEvent) {
				ev = objFactory.createEvent("EventTaskStarted");
				EventTaskStarted te = ev.getEventTaskStarted();
				te.setInfoTask( JMSTaskAdapter.createInstance(task));
				flgFound = true;
			}

			if (event instanceof TaskEndedEvent) {
				ev = objFactory.createEvent("EventTaskEnded");
				EventTaskEnded te = ev.getEventTaskEnded();
				te.setInfoTask( JMSTaskAdapter.createInstance(task));
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
			oe.setInfoLog( JMSMessageAdapter.createInstance(new SimpleMessage(logEvent.getMessage())) );  //TODO SimpleMessage, damit es übersetzbar ist. Zschimmer 30.03.2012
			flgFound = true;
		}
			
		
		if (!flgFound) {
			throw new SchedulerException("the event " + event.getClass().getName() + " is not known in class " + JMSEventAdapter.class.getName() + " - change the constructor of the class.");
		}
		
		return ev;

	}
	
}
