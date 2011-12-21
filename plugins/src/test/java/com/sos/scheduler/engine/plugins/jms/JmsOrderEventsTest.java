package com.sos.scheduler.engine.plugins.jms;

import static org.junit.Assert.assertEquals;

import java.util.Iterator;
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
import org.junit.BeforeClass;
import org.junit.Test;

import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.plugins.event.Configuration;
import com.sos.scheduler.engine.plugins.event.Connector;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.Event;


public class JmsOrderEventsTest extends SchedulerTest {
    /** Maven: mvn test -Dtest=JmsPlugInTest -DargLine=-Djms.providerUrl=tcp://localhost:61616 */
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
    private static final String providerUrl = System.getProperty("jms.providerUrl", Configuration.vmProviderUrl);

    private static final Time schedulerTimeout = Time.of(15);
    private static Configuration conf;

    private static Logger logger;

    private final Topic topic = conf.topic;
    private final TopicConnection topicConnection = conf.topicConnectionFactory.createTopicConnection();
    private final TopicSession topicSession = topicConnection.createTopicSession(false, Session.CLIENT_ACKNOWLEDGE);
    
    // Queue for collecting the fired events in the listener thread
    private final BlockingQueue<String> resultQueue = new ArrayBlockingQueue<String>(50);
    
    private final TopicSubscriber topicSubscriber;
    
    private final SchedulerObjectFactory objFactory;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
		logger = Logger.getLogger(Connector.class);
		conf = Configuration.newInstance(providerUrl);
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

    
    @Test public void test() throws Exception {
    	try {
	        controller().runSchedulerAndTerminate(schedulerTimeout, "-e -loglevel=warn");
//	        runScheduler(schedulerTimeout, "-e");
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
