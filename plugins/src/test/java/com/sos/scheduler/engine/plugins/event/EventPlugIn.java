package com.sos.scheduler.engine.plugins.event;

import static com.sos.scheduler.engine.kernel.util.XmlUtils.stringXPath;

import java.lang.reflect.Constructor;

import org.apache.log4j.Logger;
import org.w3c.dom.Element;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.AbstractEventPlugin;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import com.sos.scheduler.engine.kernel.order.ModifiableOrderEvent;
import com.sos.scheduler.engine.kernel.order.OrderState;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.JSEventBase;

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
public class EventPlugIn extends AbstractEventPlugin {
	
	private static Logger logger = Logger.getLogger(EventPlugIn.class);

	EventPlugIn(Scheduler scheduler, Element plugInElement) {
		super(scheduler, plugInElement);
		String providerUrl = stringXPath(plugInElement,	"jms/connection/@providerUrl", Configuration.vmProviderUrl);
		String persistenceDir = stringXPath(plugInElement, "jms/connection/@persistenceDirectory", Configuration.persistenceDirectory);
	}

	@Override
	public void onEvent(Event e) throws Exception {

		try {
			if (e instanceof ModifiableOrderEvent) {
				ModifiableOrderEvent orderEvent = (ModifiableOrderEvent)e;
				if (e instanceof OrderTouchedEvent) {
					logger.info("OrderTouchedEvent");
					orderEvent.getOrder().getParameters().put("myParam", "this is my Param");
				}
			}
		} catch(SchedulerException ev) {
			throw new SchedulerException(ev);
		}

	}
	
	private void setParamForState(ModifiableOrderEvent orderEvent) {
		String currentState = orderEvent.getOrder().getState().getString();
		// @TODO Die Frage ist, wie man über die Orderparameter iterieren kann, orderEvent.getOrder().getParameters()
		// liefert zwar die Parameter der Order aber es ist nicht klar, wie man sie ausliest (es sei denn,
		// man kennt den Namen)
		
	}

	public static PlugInFactory factory() {
		return new PlugInFactory() {
			@Override
			public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
				return new EventPlugIn(scheduler, plugInElement);
			}
		};
	}

}
