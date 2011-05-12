package com.sos.scheduler.engine.playground.ss;

import static com.sos.scheduler.engine.kernel.util.XmlUtils.stringXPath;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;

import org.w3c.dom.Element;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.JSEvent;


/**
 * \file JMSPlugIn.java
 * \brief JS Plugin to connect the JMS
 *  
 * \class JMSPlugIn
 * \brief JS Plugin to connect the JMS
 * 
 * \details
 * \code
  \endcode
 *
 * \version 1.0 - 12.04.2011 11:54:06
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class JMSPlugIn implements PlugIn, EventSubscriber {
    private final Scheduler scheduler;
    private final Connector connector;
    
    private SchedulerObjectFactory objFactory;

    
    JMSPlugIn(Scheduler scheduler, Element plugInElement) {
        this.scheduler = scheduler;
        String providerUrl = stringXPath(plugInElement, "jms/connection/@providerUrl", Configuration.vmProviderUrl);
        connector = Connector.newInstance(providerUrl);
        scheduler.log().info(getClass().getName() + ": providerUrl=" + providerUrl);
        //TODO PlugIns sollen eigenes PrefixLog bekommen

        //TODO get Hostname and port form JobScheduler instance an initialize the ObjectFactory with them
//        objFactory = new SchedulerObjectFactory(scheduler.getHostname(), scheduler.getTcpPort());
        objFactory = new SchedulerObjectFactory("localhost", 4444);
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
    }


    @Override public void activate() {
        scheduler.getEventSubsystem().subscribe(this);
        connector.start();
    }


    @Override public void close() {
        try {
            connector.close();
        } finally {
            scheduler.getEventSubsystem().unsubscribe(this);
        }
    }


    @Override public String getXmlState() {
        return "";
    }


    /*
     * (non-Javadoc)
     * @see com.sos.scheduler.engine.kernel.event.EventSubscriber#onEvent(com.sos.scheduler.engine.kernel.event.Event)
     * 
     * \brief delivery of the events from the scheduler kernel
     * \details
     * Each time an event was fired from the JobScheduler kernel this method will called.
     * The JobScheduler internal event will be converted in a represantation for the
     * JMS and provided as TextMessage (in XML format).
     */
    @Override public void onEvent(Event e) throws Exception {
    	
        TextMessage m = connector.newTextMessage();
        //TODO remove the following method (only for test purposes)
        String n = getEventName(e);
        
        // for test purposes only for the selected events
        //TODO execution for all events
      if (e instanceof OrderStateChangedEvent || e instanceof OrderTouchedEvent) {
	        JSEvent ev = objFactory.createEvent(e);
	        System.out.println( "publish event (" + n + "):\n" + ev.marshal() );
	        m.setText( ev.marshal() );									// provides the event as XML
	        setEventProperties(m,ev);
	        connector.publish(m);										// publish the text message (xml)
      }
    }
        
    private String getEventName(Event e) {
        if (e instanceof OrderStateChangedEvent) {
        	return "OrderStateChangedEvent";
        }
        if (e instanceof OrderTouchedEvent) {
        	return "OrderTouchedEvent";
        }
        if (e instanceof OrderFinishedEvent) {
        	return "OrderFinishedEvent";
        }
        if (e instanceof ErrorLogEvent) {
        	return "ErrorLogEvent";
        }
        return "???";
    }
    
    private void setEventProperties(Message m, JSEvent ev) throws JMSException {
        m.setStringProperty("eventName", ev.getEventName() );		// for filtering
    }

    public static PlugInFactory factory() {
        return new PlugInFactory() {
            @Override public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
                return new JMSPlugIn(scheduler, plugInElement);
            }
        };
    }
    
}
