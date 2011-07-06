package com.sos.scheduler.engine.plugins.event;

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

import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.JSHelper.Logging.Log4JHelper;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.plugins.event.Configuration;
import com.sos.scheduler.engine.plugins.event.Connector;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.Event;


public class JmsEventFilterTest extends SchedulerTest {
    /** Maven: mvn test -Dtest=JmsPlugInTest -DargLine=-Djms.providerUrl=tcp://localhost:61616 */
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
    private static final String providerUrl = System.getProperty("jms.providerUrl", Configuration.vmProviderUrl);

    private static final Time schedulerTimeout = Time.of(20);
    private static Configuration conf;

    private static Logger logger;
//    private static final boolean noLocal = false;

    private final Topic topic = conf.topic;
    private final TopicConnection topicConnection = conf.topicConnectionFactory.createTopicConnection();
    private final TopicSession topicSession = topicConnection.createTopicSession(false, Session.CLIENT_ACKNOWLEDGE);
    
    // Queue for collecting the fired events in the listener thread
    private final BlockingQueue<String> resultQueue = new ArrayBlockingQueue<String>(50);
    
    private final TopicSubscriber topicSubscriber;
    
    private final SchedulerObjectFactory objFactory;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
		// this file contains appender for ActiveMQ logging
		new Log4JHelper("src/test/resources/log4j.properties");
		logger = LoggerFactory.getLogger(Connector.class);
		conf = Configuration.newInstance(providerUrl);
	}
    
    public JmsEventFilterTest() throws Exception {
    	
    	topicSubscriber = newTopicSubscriber();
        topicSubscriber.setMessageListener(new MyListener());
        topicConnection.start();

        //TODO Connect with hostname & port from the scheduler intance
		objFactory = new SchedulerObjectFactory("localhost", 4444);
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
    }


    private TopicSubscriber newTopicSubscriber() throws JMSException {
        String messageSelector = "eventName = 'EventOrderTouched'";
        boolean noLocal = false;
    	logger.info("createSubscriber with filter: " + messageSelector);
        return topicSession.createSubscriber(topic, messageSelector, noLocal);
    }

    
    @Test
    public void test() throws Exception {
    	try {
	        runScheduler(schedulerTimeout, "-e");
	        assertEquals("EventOrderTouched",2,resultQueue.size());
		} finally {
			topicSubscriber.close();
		}
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
//throw new IllegalStateException("Nicht übersetzbarer Code, Zschimmer 2011-06-07, " + getClass());
                if (ev.getEventOrderTouched() != null) {
                	logger.info(">>>>> order " + ev.getEventOrderTouched().getOrder().getId() + " touched");
                }
                textMessage.acknowledge();
                assertEquals(getTopicname(textMessage), "com.sos.scheduler.engine.Event" );  // Erstmal ist der Klassenname vorangestellt.
                assertEquals(ev.getName(), "EventOrderTouched" );
                result = ev.getEventOrderTouched().getOrder().getId();
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
