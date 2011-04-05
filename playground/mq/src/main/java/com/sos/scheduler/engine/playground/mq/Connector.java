package com.sos.scheduler.engine.playground.mq;

import com.google.common.annotations.VisibleForTesting;
import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import javax.jms.*;
import org.apache.activemq.broker.BrokerService;


class Connector implements Closeable {
    private final BrokerService brokerService = new BrokerService();
    private final TopicConnection connection;
    private final Topic topic;
    private final TopicSession session;
    private final TopicPublisher publisher;


    Connector(Configuration c) {
        try {
            brokerService.setBrokerName(Configuration.brokerName);
            brokerService.addConnector("tcp://localhost:63102");
            connection = c.topicConnectionFactory.createTopicConnection();
            topic = c.topic;
            session = newTopicSession();
            publisher = session.createPublisher(topic);
            connection.start();
        } catch (Exception x) {
            if (x instanceof RuntimeException)  throw (RuntimeException)x;
            throw new RuntimeException(x);
        }
    }


    void setPersistenceDirectory(File dir) {
        try {
            brokerService.getPersistenceAdapter().setDirectory(dir);
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    
    void start() {
        try {
            brokerService.start();
        } catch (Exception x) { throw new RuntimeException(x); }
    }
    

    private TopicSession newTopicSession() throws JMSException {
        boolean transacted = false;
        return connection.createTopicSession(transacted, Session.AUTO_ACKNOWLEDGE);
    }


    @Override public void close() {
        try {
            try {
                connection.close();
            } catch (JMSException x) { throw new RuntimeException(x); }
            finally {
                brokerService.stop();
                //Brauche wir ein asynchrones stop()? brokerService.waitUntilStopped();
            }
        } catch (Exception x) {
            if (x instanceof RuntimeException)  throw (RuntimeException)x;
            throw new RuntimeException(x);
        }
    }


    @VisibleForTesting void waitUntilStopped() throws Exception {
        brokerService.waitUntilStopped();
        //brokerService.getPersistenceAdapter().stop();
    }


    TopicConnection getTopicConnection() {
        return connection;
    }


    Topic getTopic() {
        return topic;
    }


    TextMessage newTextMessage() {
        try {
            return session.createTextMessage();
        } catch (JMSException x) { throw new RuntimeException(x); }
    }


    void publish(Message message) {
        try {
            publisher.publish(message);
        } catch (JMSException x) { throw new RuntimeException(x); }
    }


    static Connector newInstance(String providerUrl) {
        return new Connector(Configuration.newInstance(providerUrl));
    }
}
