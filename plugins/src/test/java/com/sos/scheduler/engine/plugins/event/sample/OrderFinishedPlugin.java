package com.sos.scheduler.engine.plugins.event.sample;

import static com.sos.scheduler.engine.kernel.util.XmlUtils.stringXPath;

import org.apache.log4j.Logger;
import org.w3c.dom.Element;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.event.AbstractEventPlugin;
import com.sos.scheduler.engine.kernel.order.OrderEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.plugin.Plugin;
import com.sos.scheduler.engine.kernel.plugin.PluginFactory;

/**
 * \file OrderFinishedPlugin.java
 * \brief Simple plugin to display all Order finished
 * 
 * \class OrderFinishedPlugin
 * \brief Simple plugin to display all Order finished
 * 
 * \details
 * \code 
   \endcode
 * 
 * \version 1.0 - 18.08.2011 16:32:06
 * <div class="sos_branding">
 * <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

/**
 * The base class AbstractEventPlugin subscribes and unsubscribes the events of the JobScheduler.
 * Additionaly the interface EventSubscriber was implemented. It expects the method onEvent (see below).
 */
public class OrderFinishedPlugin extends AbstractEventPlugin {
	
	private static final Logger logger = Logger.getLogger(OrderFinishedPlugin.class);

    /**
     * The constructor of an event plugin always gets the instance of the Scheduler and a DOM element containing
     * the configuration of the plugin.
     * For test purposes some of the informations of the plugin configuration was read. To do this the class
     * com.sos.scheduler.engine.kernel.util.XmlUtils contains some static methods such as stringXPath or
     * elementXPath.
     */
	OrderFinishedPlugin(Scheduler scheduler, Element plugInElement) {
		super(scheduler, plugInElement);
		String server = stringXPath(plugInElement,	"myconfig/myserver/@host", "<unknown>");
		String port = stringXPath(plugInElement, "myconfig/myserver/@port", "<unknown>");
		logger.info("plugin configuration: server=" + server + ":" + port);
	}

	/**
	 * The method deploys the event fired by the Scheduler
	 */
	@Override
	public void onEvent(Event e) throws Exception {

		try {
			if (e instanceof OrderEvent) {
				OrderEvent orderEvent = (OrderEvent)e;
				if (e instanceof OrderFinishedEvent) {
					logger.info("Order " + orderEvent.getOrder().getId() + ": " + orderEvent.getOrder().getTitle() + " finished");
				}
			}
		} catch(SchedulerException ev) {
			throw new SchedulerException(ev);
		}

	}
	
    /**
     * Important!!
     * Any plugin has to define a static method to return a {@link PluginFactory} object.
     * This is the way to announce the plugin to the Scheduler.
     */
	public static PluginFactory factory() {
		return new PluginFactory() {
			@Override
			public Plugin newInstance(Scheduler scheduler, Element plugInElement) {
				return new OrderFinishedPlugin(scheduler, plugInElement);
			}
		};
	}

}
