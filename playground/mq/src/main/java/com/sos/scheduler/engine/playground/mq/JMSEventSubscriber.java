package com.sos.scheduler.engine.playground.mq;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import javax.jms.TextMessage;


class JMSEventSubscriber implements EventSubscriber {
    private final Connector connector;


    JMSEventSubscriber(Connector connector) {
        this.connector = connector;
    }

    
    @Override public void onEvent(Event e) throws Exception {
        TextMessage m = connector.newTextMessage();
        m.setText(e.toString());    //TODO Provisorisch, besser toXml()
        connector.publish(m);
    }
}
