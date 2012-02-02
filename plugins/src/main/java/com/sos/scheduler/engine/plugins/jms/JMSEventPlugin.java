package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.stringXPath;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.JSEvent;
import javax.inject.Inject;
import javax.jms.Message;
import javax.jms.TextMessage;
import org.apache.log4j.Logger;
import org.w3c.dom.Element;

/**
 * \file JMSPlugIn.java
 * \brief JS Plugin to connect the JMS
 * 
 * \class JMSPlugIn
 * \brief JS Plugin to connect the JMS
 * 
 * \details \code \endcode
 * 
 * \version 1.0 - 12.04.2011 11:54:06
 * <div class="sos_branding">
 * <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class JMSEventPlugin extends AbstractPlugin {
	
	private static Logger logger = Logger.getLogger(JMSEventPlugin.class);

    private final Scheduler scheduler;
	private final Connector connector;

	private SchedulerObjectFactory objFactory;

	@Inject
	JMSEventPlugin(Scheduler scheduler, Element pluginElement) {
		this.scheduler = scheduler;
		String providerUrl = stringXPath(pluginElement,	"jms/connection/@providerUrl", ActiveMQConfiguration.vmProviderUrl);
		String persistenceDir = stringXPath(pluginElement, "jms/connection/@persistenceDirectory", ActiveMQConfiguration.persistenceDirectory);
		connector = Connector.newInstance(providerUrl, persistenceDir);
		logger.info( getClass().getName() + ": providerUrl=" + providerUrl);
		scheduler.log().info("providing messages to " + providerUrl);

		logger.info("initializing SchedulerObjectFactory for " + scheduler.getHostname() + ":" + scheduler.getTcpPort());
		objFactory = new SchedulerObjectFactory(scheduler.getHostname(), scheduler.getTcpPort());
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
	}

	@Override
	public void activate() {
		super.activate();
		connector.start();
	}

	@Override
	public void close() {
		try {
			connector.close();
		} finally {
			super.close();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * com.sos.scheduler.engine.kernel.event.EventSubscriber#onEvent(com.sos
	 * .scheduler.engine.kernel.event.Event)
	 * 
	 * \brief delivery of the events from the scheduler kernel
	 * \details
	 * Each time an event was fired from the JobScheduler kernel this method will
	 * called. The JobScheduler internal event will be converted in a
	 * represantation for the JMS and provided as TextMessage (in XML format).
	 */
	@HotEventHandler
	public void handleEvent(Event e, EventSource eventSource) throws Exception {
		TextMessage m = connector.newTextMessage();
		try {
			JSEvent ev = JMSEventAdapter.createEvent(objFactory, e, eventSource);
			logger.info("publish event " + ev.getName());
			logger.debug(ev.marshal());
			m.setText(ev.marshal());
			setEventProperties(m, ev);
			connector.publish(m); // publish the text message (xml)
		} catch(SchedulerException ev) {
			logger.warn(ev.getMessage());
		}
	}

	
	/**
	 * \brief sets all additional properties of the given message
	 * \detail
	 *
	 * @param m - the message object
	 * @param ev - the event object
	 * @throws Exception
	 */
	private void setEventProperties(Message m, JSEvent ev) throws Exception {
		
		JMSMessageHelper eh = new JMSMessageHelper(m);
		eh.setJMSHeaderProperties(JMSMessageHelper.defaultEventProperties());
		
		m.setStringProperty("eventName", ev.getName()); // for filtering
		m.setStringProperty("hostname", scheduler.getHostname());
		m.setStringProperty("port", Integer.toString(scheduler.getTcpPort()) );
		m.setStringProperty("id", scheduler.getSchedulerId());
	}

}
