package com.sos.scheduler.engine.plugins.event.stepevents;

import org.apache.log4j.Logger;
import org.w3c.dom.Element;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.AbstractEventPlugin;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.order.OrderStepEndedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStepStartedEvent;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;

/**
 * \file JMSPlugIn.java \brief JS Plugin to connect the JMS
 * 
 * \class JMSPlugIn \brief JS Plugin to connect the JMS
 * 
 * \details \code \endcode
 * 
 * \version 1.0 - 12.04.2011 11:54:06 <div class="sos_branding">
 * <p>
 * (c) 2011 SOS GmbH - Berlin (<a style='color:silver'
 * href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)
 * </p>
 * </div>
 */
public class OrderStepEventsPlugIn extends AbstractEventPlugin {
	
	private static Logger logger = Logger.getLogger(OrderStepEventsPlugIn.class);

	OrderStepEventsPlugIn(Scheduler scheduler, Element plugInElement) {
		super(scheduler, plugInElement);
//		String providerUrl = stringXPath(plugInElement,	"jms/connection/@providerUrl", Configuration.vmProviderUrl);
//		String persistenceDir = stringXPath(plugInElement, "jms/connection/@persistenceDirectory", Configuration.persistenceDirectory);
	}

	@Override
	public void onEvent(Event e) throws Exception {

		try {
			if (e instanceof OrderStepStartedEvent) {
				OrderStepStartedEvent be = (OrderStepStartedEvent)e;
				logger.info("order " + be.getOrder().getId() + ": step " + be.getOrder().getState().getString() + " begins");
			}
			if (e instanceof OrderStepEndedEvent) {
				OrderStepEndedEvent ee = (OrderStepEndedEvent)e;
				logger.info("order " + ee.getOrder().getId() + ": step " + ee.getOrder().getState().getString() + " ends with " + ee.getOk());
			}
		} catch(SchedulerException ev) {
			throw new SchedulerException(ev);
		}

	}
	
//	private void setParamForState(ModifiableOrderEvent orderEvent) {
//		String currentState = orderEvent.getOrder().getState().getString();
//		// @TODO Die Frage ist, wie man über die Orderparameter iterieren kann, orderEvent.getOrder().getParameters()
//		// liefert zwar die Parameter der Order aber es ist nicht klar, wie man sie ausliest (es sei denn,
//		// man kennt den Namen)
//		
//	}

	public static PlugInFactory factory() {
		return new PlugInFactory() {
			@Override
			public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
				return new OrderStepEventsPlugIn(scheduler, plugInElement);
			}
		};
	}

}
