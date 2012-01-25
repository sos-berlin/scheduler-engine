package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.plugins.event.ActiveMQConfiguration;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSCommandUtils;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.Event;
import java.util.Iterator;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import javax.jms.*;
import org.apache.log4j.Logger;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertEquals;
import org.junit.BeforeClass;
import org.junit.Test;


public class JmsOrderEventsTest extends SchedulerTest {
    /** Maven: mvn test -Dtest=JmsPlugInTest -DargLine=-Djms.providerUrl=tcp://localhost:61616 */
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
    private final static String providerUrl = System.getProperty("jms.providerUrl", ActiveMQConfiguration.vmProviderUrl);
    private final static ActiveMQConfiguration conf = ActiveMQConfiguration.newInstance(providerUrl);
    private final static Logger logger = Logger.getLogger(JmsOrderEventsTest.class);
    private static final JSCommandUtils util = JSCommandUtils.getInstance();
    private final static String jobchain = "jmstest";

    private final Topic topic = conf.topic;
    private final TopicConnection topicConnection = conf.topicConnectionFactory.createTopicConnection();
    private final TopicSession topicSession = topicConnection.createTopicSession(false, Session.CLIENT_ACKNOWLEDGE);
    
    // Queue for collecting the fired events in the listener thread
    private final BlockingQueue<String> resultQueue = new ArrayBlockingQueue<String>(50);
    
    private final TopicSubscriber topicSubscriber;
    private final SchedulerObjectFactory objFactory;    
    private int orderFinished = 0;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
    	logger.debug("test started for class " + JmsOrderEventsTest.class.getSimpleName());
	}
    
    
    public JmsOrderEventsTest() throws Exception {
    	
    	topicSubscriber = newTopicSubscriber();
        topicSubscriber.setMessageListener(new MyListener());
        topicConnection.start();

        //TODO Connect with hostname & port from the scheduler intance
		objFactory = new SchedulerObjectFactory("localhost", 4444);
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
    }


    private TopicSubscriber newTopicSubscriber() throws JMSException {
        String messageSelector = null;
        boolean noLocal = false;
    	logger.info("createSubscriber with filter: " + messageSelector);
        return topicSession.createSubscriber(topic, messageSelector, noLocal);
    }

    
    @Test
    public void test() throws Exception {
    	try {
    		controller().activateScheduler();
	        controller().scheduler().executeXml( util.buildCommandAddOrder(jobchain, "order1").getCommand() );
	        controller().scheduler().executeXml( util.buildCommandAddOrder(jobchain, "order2").getCommand() );
    		controller().waitForTermination(shortTimeout);
	        assertEvent("EventOrderTouched",2);
	        assertEvent("EventOrderStateChanged",4);
	        assertEvent("EventOrderFinished",2);
    	} finally {
    		topicSubscriber.close();
    	}
    }
    
    private void assertEvent(String eventName, int exceptedEvents) {
    	Iterator<String> it = resultQueue.iterator();
    	int cnt = 0;
    	while (it.hasNext()) {
    		String e = it.next();
    		if(e.equals(eventName)) cnt++;
    	}
        assertEquals(eventName,exceptedEvents,cnt);
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
            	logger.debug("XML-Content=" + xmlContent);
                Event ev = (Event)objFactory.unMarshall(xmlContent);		// get the event object
            	logger.info("subscribe " + ev.getName());
                if (ev.getEventOrderTouched() != null) {
                	logger.info(">>>>> order " + ev.getEventOrderTouched().getInfoOrder().getId() + " touched");
                }
                if (ev.getEventOrderStateChanged() != null) {
                	logger.info(">>>>> order " + ev.getEventOrderStateChanged().getInfoOrder().getId() + " goes to state " + ev.getEventOrderStateChanged().getInfoOrder().getState() + " (previous State: " + ev.getEventOrderStateChanged().getPreviousState() + ")");
                }
                if (ev.getEventOrderFinished() != null) {
                	logger.info(">>>>> order " + ev.getEventOrderFinished().getInfoOrder().getId() + " finished.");
                }
                textMessage.acknowledge();
                assertEquals(getTopicname(textMessage), "com.sos.scheduler.engine.Event" );  // Erstmal ist der Klassenname vorangestellt.
                result = ev.getName();
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
