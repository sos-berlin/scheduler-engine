package com.sos.scheduler.engine.plugins.jms;

import static com.sos.scheduler.engine.kernel.util.XmlUtils.stringXPath;

import javax.jms.Message;
import javax.jms.TextMessage;

import org.apache.log4j.Logger;
import org.w3c.dom.Element;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.JSEvent;

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
/**
 * \file JMSEventPlugIn.java
 * \brief 
 *  
 * \class JMSEventPlugIn
 * \brief 
 * 
 * \details
 *
 * \code
  \endcode
 *
 * \author schaedi
 * \version 1.0 - 17.05.2011 15:34:17
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
/**
 * \file JMSEventPlugIn.java
 * \brief 
 *  
 * \class JMSEventPlugIn
 * \brief 
 * 
 * \details
 *
 * \code
  \endcode
 *
 * \version 1.0 - 17.05.2011 15:34:23
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class JMSEventPlugIn implements PlugIn, EventSubscriber {
	private final Scheduler scheduler;
	private final Connector connector;

	private SchedulerObjectFactory objFactory;
	private static Logger logger = Logger.getLogger(JMSEventPlugIn.class
			.getName());

	JMSEventPlugIn(Scheduler scheduler, Element plugInElement) {
		this.scheduler = scheduler;
		String providerUrl = stringXPath(plugInElement,
				"jms/connection/@providerUrl", Configuration.vmProviderUrl);
		String persistenceDir = stringXPath(plugInElement,
				"jms/connection/@persistenceDirectory",
				Configuration.persistenceDirectory);
		connector = Connector.newInstance(providerUrl, persistenceDir);
		scheduler.log().info(
				getClass().getName() + ": providerUrl=" + providerUrl);
		// TODO PlugIns sollen eigenes PrefixLog bekommen

		logger.info("initializing SchedulerObjectFactory for " + scheduler.getHostname() + ":" + scheduler.getTcpPort());
		objFactory = new SchedulerObjectFactory(scheduler.getHostname(), scheduler.getTcpPort());
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
	}

	@Override
	public void activate() {
		scheduler.getEventSubsystem().subscribe(this);
		connector.start();
	}

	@Override
	public void close() {
		try {
			connector.close();
		} finally {
			scheduler.getEventSubsystem().unsubscribe(this);
		}
	}

	@Override
	public String getXmlState() {
		return "";
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * com.sos.scheduler.engine.kernel.event.EventSubscriber#onEvent(com.sos
	 * .scheduler.engine.kernel.event.Event)
	 * 
	 * \brief delivery of the events from the scheduler kernel \details Each
	 * time an event was fired from the JobScheduler kernel this method will
	 * called. The JobScheduler internal event will be converted in a
	 * represantation for the JMS and provided as TextMessage (in XML format).
	 */
	@Override
	public void onEvent(Event e) throws Exception {

		TextMessage m = connector.newTextMessage();

		// for test purposes only for the selected events
		// TODO execution for all events
		if (e instanceof OrderStateChangedEvent
				|| e instanceof OrderTouchedEvent
				|| e instanceof OrderFinishedEvent) {
			try {
//				JSEvent ev = objFactory.createEvent(e);
				JSEvent ev = JMSEventAdapter.createEvent(objFactory, e);
				logger.info("publish event " + ev.getName());
				logger.debug(ev.marshal());
				m.setText(ev.marshal());
				setEventProperties(m, ev);
				connector.publish(m); // publish the text message (xml)
			} catch(SchedulerException ev) {
				throw new SchedulerException(ev);
			}
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
		
	}

	public static PlugInFactory factory() {
		return new PlugInFactory() {
			@Override
			public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
				return new JMSEventPlugIn(scheduler, plugInElement);
			}
		};
	}

}
