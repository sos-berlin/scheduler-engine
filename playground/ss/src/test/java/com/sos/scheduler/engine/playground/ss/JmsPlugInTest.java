package com.sos.scheduler.engine.playground.ss;

import com.sos.scheduler.engine.kernelcpptest.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.playground.ss.Configuration;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.Event;
//import com.sos.VirtualFileSystem.Interfaces.ISOSVirtualFile;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import javax.jms.Session;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicConnection;
import javax.jms.TopicSession;
import javax.jms.TopicSubscriber;

import org.junit.Ignore;
import org.junit.Test;
//import org.apache.log4j.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public class JmsPlugInTest extends SchedulerTest {
    /** Maven: mvn test -Dtest=JmsPlugInTest -DargLine=-Djms.providerUrl=tcp://localhost:61616 */
    private static final String providerUrl = System.getProperty("jms.providerUrl", Configuration.vmProviderUrl);

    private static final Time schedulerTimeout = Time.of(15);
    private static final Configuration conf = Configuration.newInstance(providerUrl);
    private static final Logger logger = LoggerFactory.getLogger(JmsPlugInTest.class);
    private static final boolean noLocal = false;

    private final Topic topic = conf.topic;
    private final TopicConnection topicConnection = conf.topicConnectionFactory.createTopicConnection();
    private final TopicSession topicSession = topicConnection.createTopicSession(false, Session.CLIENT_ACKNOWLEDGE);
    private final BlockingQueue<Boolean> resultQueue = new ArrayBlockingQueue<Boolean>(50);
    
    private final TopicSubscriber topicSubscriber;
    
    private final SchedulerObjectFactory objFactory;
    
    public JmsPlugInTest() throws Exception {
    	topicSubscriber = newTopicSubscriber();
        topicSubscriber.setMessageListener(new MyListener());
        topicConnection.start();
        
		objFactory = new SchedulerObjectFactory("localhost", 4444);
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
    }


    private TopicSubscriber newTopicSubscriber() throws JMSException {
//        String messageSelector = null;
        String messageSelector = "eventName = 'OrderTouchedEvent'";
        boolean noLocal = false;
        return topicSession.createSubscriber(topic, messageSelector, noLocal);
    }

    
//    @Test public void dummy() throws Exception {
//    	System.err.println("this is a summy");
//    }
    
    @Test public void test() throws Exception {
        runScheduler(schedulerTimeout, "-e");
        assertThat(resultQueue.poll(0, TimeUnit.SECONDS), equalTo(true));
    }


    private class MyListener implements javax.jms.MessageListener {
        @Override public void onMessage(Message message) {
            // Läuft in einem Thread von JMS
            boolean result = false;
            try {
                TextMessage textMessage = (TextMessage) message;
//                showMessageHeader(textMessage);
                String xmlContent = textMessage.getText();
            	System.err.println("\nsubscribe:\n" + xmlContent );
                Event ev = (Event)objFactory.unMarshall(xmlContent);		// get the event object
                if (ev.getOrderTouchedEvent() != null) {
                	System.err.println(">>>>> order " + ev.getOrderTouchedEvent().getOrder().getId() + " touched");
                }
                if (ev.getOrderStateChangeEvent() != null) {
                	System.err.println(">>>>> order " + ev.getOrderStateChangeEvent().getOrder().getId() + " goes to state " + ev.getOrderStateChangeEvent().getState() + " (previous State: " + ev.getOrderStateChangeEvent().getPreviousState() + ")");
                }
                if (ev.getOrderFinishedEvent() != null) {
                	System.err.println(">>>>> order " + ev.getOrderStateChangeEvent().getOrder().getId() + " goes to state " + ev.getOrderStateChangeEvent().getState() + " (previous State: " + ev.getOrderStateChangeEvent().getPreviousState() + ")");
                }
                textMessage.acknowledge();
//                assertThat(textMessage.getText(), startsWith("com.sos.scheduler.engine."));  // Erstmal ist der Klassenname vorangestellt.
                result = true;
            }
            catch (JMSException x) { throw new RuntimeException(x); }
            finally {
                resultQueue.add(result);
            }
        }
    }
    
    private void showMessageHeader(Message m) throws JMSException {
    	System.out.println("getJMSCorrelationID=" + m.getJMSCorrelationID() );
    	System.out.println("getJMSDeliveryMode (persistent/non persistent)=" + m.getJMSDeliveryMode() );
    	System.out.println("getJMSExpiration=" + m.getJMSExpiration() );
    	System.out.println("getJMSMessageID=" + m.getJMSMessageID() );
    	System.out.println("getJMSPriority=" + m.getJMSPriority() );
    	System.out.println("getJMSTimestamp=" + m.getJMSTimestamp() );
    	System.out.println("getJMSDestination=" + m.getJMSType() );
    	Topic t = (Topic)m.getJMSDestination();
    	System.out.println("getJMSDestination (Topicname)=" + t.getTopicName() );
    	System.out.println("getJMSRedelivered=" + m.getJMSRedelivered() );
    	System.out.println("getJMSReplyTo=" + m.getJMSReplyTo().toString() );
    }
}
