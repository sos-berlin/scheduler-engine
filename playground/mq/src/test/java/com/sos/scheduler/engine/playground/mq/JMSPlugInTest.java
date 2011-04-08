//package com.sos.scheduler.engine.playground.mq;
//
//import com.google.common.io.Files;
//import com.sos.scheduler.engine.kernel.event.AbstractEvent;
//import java.io.File;
//import java.util.concurrent.ArrayBlockingQueue;
//import java.util.concurrent.BlockingQueue;
//import java.util.concurrent.TimeUnit;
//import javax.jms.*;
//import org.junit.*;
//import static org.hamcrest.MatcherAssert.assertThat;
//import static org.hamcrest.Matchers.*;
//
////TODO Schlecht: Für BrokerService "localhost" wird Verzeichnis activemq-data im Arbeitverzeichnis angelegt. Wie ändern wir das? Wir brauchen den nicht.
//
//public class JMSPlugInTest {
//    private final File tmpDir = Files.createTempDir();
//    private final Connector connector = Connector.newInstance(Configuration.vmProviderUrl);
//    private final Topic topic = connector.getTopic();
//    private final TopicConnection topicConnection = connector.getTopicConnection();
//    private final TopicSession topicSession = topicConnection.createTopicSession(false, Session.AUTO_ACKNOWLEDGE);
//    private final TopicSubscriber subscriber = newTopicSubscriber();
//    private final BlockingQueue<Boolean> resultQueue = new ArrayBlockingQueue<Boolean>(1);
//
//
//    public JMSPlugInTest() throws Exception {
//        connector.setPersistenceDirectory(tmpDir);
//        connector.start();
//    }
//
//
//    @After public void close() throws Exception {
//        try {
//            connector.close();
//        } finally {
//            connector.waitUntilStopped();
//            //TODO Nicht löschbar: Files.deleteRecursively(tmpDir);
//        }
//    }
//
//
//    @Test public void testOnEvent() throws Exception {
//        eventSubscriber.onEvent(new MockEvent());
//        startListener();
//        assertThat(resultQueue.poll(3, TimeUnit.SECONDS), equalTo(true));
//    }
//
//
//    private void startListener() throws JMSException {
//        subscriber.setMessageListener(new MyListener());
//    }
//
//
//    private TopicSubscriber newTopicSubscriber() throws JMSException {
//        String messageSelector = null;
//        boolean noLocal = false;
//        return topicSession.createSubscriber(topic, messageSelector, noLocal);
//    }
//
//
//    private class MyListener implements javax.jms.MessageListener {
//        @Override public void onMessage(Message message) {
//            // Läuft in einem Thread von JMS
//            boolean result = false;
//            try {
//                TextMessage textMessage = (TextMessage) message;
//                assertThat(textMessage.getText(), containsString(MockEvent.class.getName()));
//                result = true;
//            }
//            catch (JMSException x) { throw new RuntimeException(x); }
//            finally {
//                resultQueue.add(result);
//            }
//        }
//    }
//
//
//    private static class MockEvent extends AbstractEvent {}
//}
