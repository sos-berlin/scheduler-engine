package com.sos.scheduler.engine.plugins.event.sample;

import static com.sos.scheduler.engine.kernel.util.XmlUtils.stringXPath;

import org.apache.log4j.Logger;
import org.w3c.dom.Element;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.AbstractEventPlugin;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.order.OrderEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import com.sos.scheduler.engine.plugins.event.Configuration;

/**
 * \file OrderFinishedPlugIn.java 
 * \brief Simple plugin to display all Order finished
 * 
 * \class OrderFinishedPlugIn
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
 * Die Basisklasse AbstractEventPlugin abonniert die Events des Schedulers beim Start und meldet sie
 * bei Beendigung des Schedulers ordnungsgemäß wieder ab. Außerdem implementiert sie das Interface 
 * EventSubscriber, welches die Methode onEvent (s.u.) erwartet.
 */
public class OrderFinishedPlugIn extends AbstractEventPlugin {
	
	private static Logger logger = Logger.getLogger(OrderFinishedPlugIn.class);

    /**
     * Der Konstruktor eines event plugin erhält immer die Instanz des Schedulers und ein DOM Element, welches
     * die Konfiguration des plugin enthält.
     * Zu Demonstartionszwecken werden einige Informationen der plugin Konfiguration gelesen. Dazu gibt es in der 
     * Klasse com.sos.scheduler.engine.kernel.util.XmlUtils einige statische Hilfsmethoden wie stringXPath oder
     * elementXPath.
     */
	OrderFinishedPlugIn(Scheduler scheduler, Element plugInElement) {
		super(scheduler, plugInElement);
		String server = stringXPath(plugInElement,	"myconfig/myserver/@host", "<unknown>");
		String port = stringXPath(plugInElement, "myconfig/myserver/@port", "<unknown>");
		logger.info("plugin configuration: server=" + server + ":" + port);
	}

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
     * Wichtig !!
     * Jedes Plugin muß eine statische Methode besitzen, die eine PlugInFactory liefert.
     * Damit wird das plugin dem Scheduler bekannt gemacht.
     */
	public static PlugInFactory factory() {
		return new PlugInFactory() {
			@Override
			public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
				return new OrderFinishedPlugIn(scheduler, plugInElement);
			}
		};
	}

}
