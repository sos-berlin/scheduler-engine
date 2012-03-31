package com.sos.scheduler.engine.playground.mq;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.data.event.Event;
import org.w3c.dom.Element;

import javax.inject.Inject;
import javax.jms.JMSException;
import javax.jms.TextMessage;

import static com.sos.scheduler.engine.kernel.util.XmlUtils.stringXPath;

public class JMSPlugin extends AbstractPlugin {
    private final Connector connector;

    @Inject public JMSPlugin(PrefixLog log, Element pluginElement) {
        String providerUrl = stringXPath(pluginElement, "jms/connection/@providerUrl", Configuration.vmProviderUrl);
        connector = Connector.newInstance(providerUrl);
        log.info(JMSPlugin.class.getName() + ": providerUrl=" + providerUrl);
    }

    @Override public final void activate() {
        connector.start();
    }

    @Override public final void close() {
        connector.close();
    }

    @EventHandler public final void handleEvent(Event e) throws JMSException {
        TextMessage m = connector.newTextMessage();
        m.setText(e.toString());    //TODO Provisorisch, besser toXml()
        connector.publish(m);
    }
}
