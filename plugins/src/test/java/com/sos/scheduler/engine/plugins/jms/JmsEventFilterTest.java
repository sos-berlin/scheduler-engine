package com.sos.scheduler.engine.plugins.jms;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicConnection;
import javax.jms.TopicSession;
import javax.jms.TopicSubscriber;

import org.apache.log4j.Logger;
import org.junit.Test;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.plugins.event.ActiveMQConfiguration;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSCommandUtils;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.Event;


public class JmsEventFilterTest extends SchedulerTest {
    /** Maven: mvn test -Dtest=JmsPlugInTest -DargLine=-Djms.providerUrl=tcp://localhost:61616 */
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
    private static final String providerUrl = System.getProperty("jms.providerUrl", ActiveMQConfiguration.vmProviderUrl);
    private static final ActiveMQConfiguration conf = ActiveMQConfiguration.newInstance(providerUrl);
    private static final Logger logger = Logger.getLogger(JmsEventFilterTest.class);
    private static final JSCommandUtils util = JSCommandUtils.getInstance();

    private final Topic topic = conf.topic;
    private final TopicConnection topicConnection = conf.topicConnectionFactory.createTopicConnection();
    private final TopicSession topicSession = topicConnection.createTopicSession(false, Session.CLIENT_ACKNOWLEDGE);
    private final TopicSubscriber topicSubscriber;
    
    private final String eventToProvide = "EventOrderTouched";
    private final String jobchain = "jmstest";
    private int orderFinished = 0;
    
    // Queue for collecting the fired events in the listener thread
    private final BlockingQueue<String> resultQueue = new ArrayBlockingQueue<String>(50);
    
    private final SchedulerObjectFactory objFactory;
    
    public JmsEventFilterTest() throws Exception {
    	
    	topicSubscriber = newTopicSubscriber();
        topicSubscriber.setMessageListener(new MyListener());
        topicConnection.start();

        //TODO Connect with hostname & port from the scheduler intance
		objFactory = new SchedulerObjectFactory("localhost", 4444);
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
    }


    private TopicSubscriber newTopicSubscriber() throws JMSException {
        String messageSelector = "eventName = '" + eventToProvide + "'";
        boolean noLocal = false;
    	logger.info("createSubscriber with filter: " + messageSelector);
        return topicSession.createSubscriber(topic, messageSelector, noLocal);
    }

    
    @Test
    public void test() throws Exception {
    	try {
	        controller().activateScheduler("-e -log-level=warn");
	        controller().scheduler().executeXml( util.buildCommandAddOrder(jobchain, "order1").getCommand() );
	        controller().scheduler().executeXml( util.buildCommandAddOrder(jobchain, "order2").getCommand() );
	        controller().waitForTermination(shortTimeout);
	        assertEquals("two events of " + eventToProvide + " expected",2,resultQueue.size());
	        assertTrue("'order1' is not in result queue",resultQueue.contains("order1"));
	        assertTrue("'order2' is not in result queue",resultQueue.contains("order2"));
		} finally {
			topicSubscriber.close();
		}
    }
	
    @HotEventHandler
    public void handleOrderEnd(OrderFinishedEvent e, UnmodifiableOrder o) throws Exception {
    	logger.debug("ORDERFINISHED: " + o.getId().getString());
    	orderFinished++;
    	if (orderFinished == 2)
    		controller().scheduler().terminate();
    }
    
    private class MyListener implements javax.jms.MessageListener {

        // runs in an own thread
    	@Override
        public void onMessage(Message message) {
            String result = "<unknown event>";
            try {
                TextMessage textMessage = (TextMessage) message;
                showMessageHeader(textMessage);
                String xmlContent = textMessage.getText();
                Event ev = (Event)objFactory.unMarshall(xmlContent);		// get the event object
            	logger.info("subscribe " + ev.getName());
            	logger.debug(xmlContent);
                if (ev.getEventOrderTouched() == null) {
                	throw new SchedulerException(ev.getName() + " not expected - expected event is " + eventToProvide);
                }
                textMessage.acknowledge();
                assertEquals(getTopicname(textMessage), "com.sos.scheduler.engine.Event" );  // Erstmal ist der Klassenname vorangestellt.
                result = ev.getEventOrderTouched().getInfoOrder().getId();
            }
            catch (JMSException x) { throw new RuntimeException(x); }
            finally {
                try {
					resultQueue.put(result);
				} catch (InterruptedException e) {
					logger.error(e.getMessage());
				}
            }
        }
    }
    
    private void showMessageHeader(Message m) throws JMSException {
    	logger.debug("getJMSCorrelationID=" + m.getJMSCorrelationID() );
    	logger.debug("getJMSDeliveryMode (persistent/non persistent)=" + m.getJMSDeliveryMode() );
    	logger.debug("getJMSExpiration=" + m.getJMSExpiration() );
    	logger.debug("getJMSMessageID=" + m.getJMSMessageID() );
    	logger.debug("getJMSPriority=" + m.getJMSPriority() );
    	logger.debug("getJMSTimestamp=" + m.getJMSTimestamp() );
    	logger.debug("getJMSDestination=" + m.getJMSType() );
    	logger.debug("getJMSRedelivered=" + m.getJMSRedelivered() );
    	logger.debug("getJMSDestination (Topicname)=" + getTopicname(m) );
    	if (m.getJMSReplyTo()!=null) logger.debug("getJMSReplyTo=" + m.getJMSReplyTo().toString() );
    }
    
    private String getTopicname(Message m) throws JMSException {
    	Topic t = (Topic)m.getJMSDestination();
    	return (t.getTopicName()!=null) ? t.getTopicName() : "???";
    }

}
