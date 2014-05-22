package com.sos.scheduler.engine.plugins.jms;

import static com.sos.scheduler.engine.common.xml.XmlUtils.stringXPath;
import static java.net.InetAddress.getLocalHost;

import java.util.ArrayList;
import java.util.List;

import javax.inject.Inject;
import javax.inject.Named;
import javax.jms.Message;
import javax.jms.TextMessage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Element;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.ImmutableSet;
import com.sos.scheduler.engine.data.configuration.EngineJacksonConfiguration;
import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.Plugins;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;

/**
 * JS Plugin to connect the JMS
 */
public class JMSEventPlugin extends AbstractPlugin implements EventHandlerAnnotated {
	
	private static final Logger logger = LoggerFactory.getLogger(JMSEventPlugin.class);

    private final SchedulerConfiguration schedulerConfiguration;
    private final SchedulerConfiguration configuration;
	private final Connector connector;
    private final ObjectMapper mapper;
    
    private final List<Package> publicEventPackages = new ArrayList<Package>();
    private final ImmutableSet<String> defautlBaseEventPackages =  ImmutableSet.of(
        "com.sos.scheduler.engine.data"
        // here you can define additional packages for events to provide
    );

	@Inject
	private JMSEventPlugin(
            SchedulerConfiguration c,
            PrefixLog prefixLog,
            SchedulerConfiguration configuration,
            @Named(Plugins.configurationXMLName) Element pluginElement) {
        this.schedulerConfiguration = c;
        this.configuration = configuration;
		String providerUrl = stringXPath(pluginElement,	"jms/connection/@providerUrl", ActiveMQConfiguration.vmProviderUrl);
		String persistenceDir = stringXPath(pluginElement, "jms/connection/@persistenceDirectory", ActiveMQConfiguration.persistenceDirectory);
		connector = Connector.newInstance(providerUrl, persistenceDir);
		logger.info( getClass().getName() + ": providerUrl=" + providerUrl);
		prefixLog.info("Providing messages to " + providerUrl);
        mapper = EngineJacksonConfiguration.newObjectMapper();
        registerDefaultEventPackages();
	}
    
    private void registerDefaultEventPackages() {
        for(String basePackage : defautlBaseEventPackages) {
            for(Package p : Package.getPackages()) {
                if (p.getName().startsWith(basePackage))
                    registerPublicEventPackage(p);
            }
        }
    }

    public void registerPublicEventPackage(Package packageToRegister) {
        publicEventPackages.add(packageToRegister);
    }

	@Override
	public void onActivate() {
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
	 * representation for the JMS and provided as TextMessage (in Json format).
	 */
	@HotEventHandler
	public void handleEvent(Event e) throws Exception {
		TextMessage m = connector.newTextMessage();
		try {
            if (isPublicEvent(e)) {
                String jsonText = mapper.writeValueAsString(e);
                // logger.info("publish event: " + jsonText);
                m.setText(jsonText);
                setEventProperties(m, e);
                connector.publish(m); // publish the text message (json)
            } else {
                logger.warn("no public event: " + e.getClass().getSimpleName());
            }
		} catch(SchedulerException ev) {
			logger.warn(ev.getMessage());
		}
	}
    
    private boolean isPublicEvent(Event e) {
        boolean result = false;
        String packageNameOfEvent = e.getClass().getPackage().getName();
        for(Package p : publicEventPackages) {
            if (packageNameOfEvent.startsWith(p.getName())) {
                result = true;
                break;
            }
        }
        return result;
    }

	
	/**
	 * \brief sets all additional properties of the given message
	 * \detail
	 *
	 *
     * @param message - the message object
     * @param event - the event class
     * @throws Exception
	 */
	private void setEventProperties(Message message, Event event) throws Exception {
        message.setIntProperty("JMSPriority", 20);
        message.setStringProperty("eventName", event.getClass().getSimpleName()); // for filtering
        message.setStringProperty("eventFullName", event.getClass().getName()); // for filtering
		message.setStringProperty("hostname", getLocalHost().getHostName());
		message.setStringProperty("port", Integer.toString(schedulerConfiguration.tcpPort()));
		message.setStringProperty("id", configuration.schedulerId().string());
	}
}
