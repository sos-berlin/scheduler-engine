package com.sos.scheduler.engine.playground.mq;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


public class JMSPlugIn implements PlugIn {
    private final Scheduler scheduler;
    private final Connector connector;
    private final JMSEventSubscriber eventSubscriber;

    
    JMSPlugIn(Scheduler scheduler, Element plugInElement) {
        this.scheduler = scheduler;
        String providerUrl = stringXPath(plugInElement, "jms/connection/@providerUrl", Configuration.vmProviderUrl);
        connector = Connector.newInstance(providerUrl);
        scheduler.log().info(getClass().getName() + ": providerUrl=" + providerUrl);
        eventSubscriber = new JMSEventSubscriber(connector);
        scheduler.getEventSubsystem().subscribe(eventSubscriber);
    }


    @Override public void activate() {
        connector.start();
    }
    

    @Override public void close() {
        try {
            connector.close();
        } finally {
            scheduler.getEventSubsystem().unsubscribe(eventSubscriber);
        }
    }


    @Override public String getXmlState() {
        return "";
    }


    @Override public String toString() {
        return "PlugIn " + getClass().getName();
    }

    
    public static PlugInFactory factory() {
        return new PlugInFactory() {
            @Override public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
                return new JMSPlugIn(scheduler, plugInElement);
            }
        };
    }
}
