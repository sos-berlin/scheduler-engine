package com.sos.scheduler.engine.playground.mq;

import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import javax.jms.*;
import org.apache.activemq.broker.BrokerService;


class Connector implements Closeable {
//    private final BrokerService brokerService = new BrokerService();
    // BrokerService auskommentiert, weil der BrokerService am Ende nicht ordentlich abgeräumt wird, dann liegt noch eine MBean herum:
    // "javax.management.InstanceAlreadyExistsException: org.apache.activemq:BrokerName=com.sos.scheduler,Type=Broker" in addMBean()
    // ActiveMQ hat einen automatisch startenden BrokerService unter dem Namen "localhost".
    // Der legt im Arbeitsverzeichnis seine Datenbank an, im Unterverzeichnis activemq-data.

    private final TopicConnection connection;
    private final Topic topic;
    private final TopicSession session;
    private final TopicPublisher publisher;


    Connector(Configuration c) {
        try {
            //brokerService.setBrokerName(Configuration.brokerName);
            //brokerService.addConnector("tcp://localhost:63102");
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


    private TopicSession newTopicSession() throws JMSException {
        boolean transacted = false;
        return connection.createTopicSession(transacted, Session.AUTO_ACKNOWLEDGE);
    }


    void setPersistenceDirectory(File dir) {
//        try {
//            brokerService.getPersistenceAdapter().setDirectory(dir);
//        } catch (IOException x) { throw new RuntimeException(x); }
    }

    
    void start() {
//        try {
//            brokerService.start();
//        } catch (Exception x) { throw new RuntimeException(x); }
    }


    public void stop() {
        try {
            try {
                connection.close();
            } catch (JMSException x) { throw new RuntimeException(x); }
            finally {
//                brokerService.stop();
            }
        } catch (Exception x) {
            if (x instanceof RuntimeException)  throw (RuntimeException)x;
            throw new RuntimeException(x);
        }
    }

    
    @Override public void close() {
        stop();
//        brokerService.waitUntilStopped();
    }


    void waitUntilStopped() throws Exception {
//        brokerService.waitUntilStopped();
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
