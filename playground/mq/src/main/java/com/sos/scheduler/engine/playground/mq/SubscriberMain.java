package com.sos.scheduler.engine.playground.mq;

// Start mit
// mvn exec:java -Dexec.mainClass=com.sos.scheduler.engine.kernelcpptest.plugin.jms.SubscriberMain

import java.io.PrintStream;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicConnection;
import javax.jms.TopicSession;
import javax.jms.TopicSubscriber;


public final class SubscriberMain {
    /** Maven: mvn test -Dtest=JmsPluginTest -DargLine=-Djms.providerUrl=http://localhost:61616 */
    private static final String providerUrl = System.getProperty("jms.providerUrl", "tcp://localhost:61616");
    private static final Configuration conf = Configuration.newInstance(providerUrl);
    private static final PrintStream err = System.err;
//    private static final Logger logger = Logger.getLogger(SubscriberMain.class);

    private final Topic topic = conf.topic;
    private final TopicConnection topicConnection = conf.topicConnectionFactory.createTopicConnection();
    private final TopicSession topicSession = topicConnection.createTopicSession(false, Session.AUTO_ACKNOWLEDGE);
    private final TopicSubscriber topicSubscriber = newTopicSubscriber();


    public SubscriberMain() throws Exception {
    }


    public void run() throws Exception {
        MyListener listener = new MyListener();
        topicSubscriber.setMessageListener(listener);
        topicConnection.start();
        err.println("Listening ...");
        Thread.sleep(Integer.MAX_VALUE);
    }

    
    public void close() throws Exception {
        topicConnection.close();
    }


    private TopicSubscriber newTopicSubscriber() throws JMSException {
        String messageSelector = null;
        boolean noLocal = false;
        return topicSession.createSubscriber(topic, messageSelector, noLocal);
    }


    private static class MyListener implements javax.jms.MessageListener {
        @Override public void onMessage(Message message) {
            // Läuft in einem Thread von JMS
            try {
                TextMessage textMessage = (TextMessage)message;
                err.println(textMessage.getText());
            } catch (Throwable x) {
                err.println("ERROR " + x);
                throw new RuntimeException(x);
            }
        }
    }


    public static void main(String[] args) {
        try {
            SubscriberMain m = new SubscriberMain();
            try {
                m.run();
            } finally {
                m.close();
            }
        } catch (Throwable x) {
            err.println(x);
            throw new RuntimeException(x);
        }
    }
}
