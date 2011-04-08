package com.sos.scheduler.engine.playground.mq;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import javax.jms.TextMessage;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


public class JMSPlugIn implements PlugIn, EventSubscriber {
    private final Scheduler scheduler;
    private final Connector connector;

    
    JMSPlugIn(Scheduler scheduler, Element plugInElement) {
        this.scheduler = scheduler;
        String providerUrl = stringXPath(plugInElement, "jms/connection/@providerUrl", Configuration.vmProviderUrl);
        connector = Connector.newInstance(providerUrl);
        scheduler.log().info(getClass().getName() + ": providerUrl=" + providerUrl);
        //TODO PlugIns sollen eigenes PrefixLog bekommen
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


    @Override public void onEvent(Event e) throws Exception {
        TextMessage m = connector.newTextMessage();
        m.setText(e.toString());    //TODO Provisorisch, besser toXml()
        connector.publish(m);
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
