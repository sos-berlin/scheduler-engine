package com.sos.scheduler.engine.kernelcpptest.jira.js461;

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

import com.sos.scheduler.engine.kernel.test.SuperSchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.plugins.event.Configuration;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.Event;

/**
 * \file JS461Test.java
 * \brief js-461: modify order set state to endstate 
 *  
 * \class JS461Test
 * \brief js-461: modify order set state to endstate 
 * 
 * \details
 * The sample configuration contains a jobchain with three nodes. Running this test the chain starts and should be suspend at the second node
 * (job js-461-2), because the job ends with error. The test set the state of the suspended order to "success" and resumed it.
 * The test expects the following events fired by the scheduler:
 * - EventOrderSuspended if the job job js-461-2 ends with error
 * - EventOrderResumed if the order was set to suspended="no"
 * - EventOrderFinished because the order resumed in the "success" state 
 *
 * \code
  \endcode
 *
 * \author ss
 * \version 1.0 - 25.05.2011 19:07:21
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class JS461Test extends SuperSchedulerTest {

	private final String ORDER = "js-461-order";
	private final String JOB_CHAIN = "js-461";
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
    private static final String providerUrl = System.getProperty("jms.providerUrl", Configuration.vmProviderUrl);

    private static final Time schedulerTimeout = Time.of(10);
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
		logger = LoggerFactory.getLogger(JS461Test.class);
		conf = Configuration.newInstance(providerUrl);
	}
    
    public JS461Test() throws Exception {
    	
    	topicSubscriber = newTopicSubscriber();
        topicSubscriber.setMessageListener(new MyListener());
        topicConnection.start();

		objFactory = new SchedulerObjectFactory("localhost", 4444);
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
    }


    /**
     * \brief TopicSubscriber erzeugen nur für OrderFinishedEvent erzeugen
     *
     * @return
     * @throws JMSException
     */
    private TopicSubscriber newTopicSubscriber() throws JMSException {
        String messageSelector = "eventName = 'EventOrderResumed' or eventName = 'EventOrderSuspended' or eventName = 'EventOrderFinished'";
        boolean noLocal = false;
        logger.debug("eventFilter is: " + messageSelector);
        return topicSession.createSubscriber(topic, messageSelector, noLocal);
    }

    
    /**
     * \brief Test ausführen
     * \detail
     * Es werden 2 OrderFinishedEvents erwartet (und sonst nichts)
     *
     * @throws Exception
     */
    @Test 
    public void test() throws Exception {
        runScheduler(schedulerTimeout, "-e");
        assertEvent("EventOrderSuspended",1);										// one order has to end with 'success'
        assertEvent("EventOrderResumed",1);										// one order has to end with 'success'
        assertEvent("EventOrderFinished",1);										// one order has to end with 'success'
        assertEquals("total number of events",3,resultQueue.size());	// totaly 4 OrderFinishedEvents
    }
    
    private void assertEvent(String eventName, int exceptedHits) {
    	Iterator<String> it = resultQueue.iterator();
    	int cnt = 0;
    	while (it.hasNext()) {
    		String e = it.next();
    		if(e.equals(eventName)) cnt++;
    	}
        assertEquals("event '" + eventName + "'",exceptedHits,cnt);
    }
    
    private class MyListener implements javax.jms.MessageListener {

        // runs in an own thread
    	@Override
        public void onMessage(Message message) {
            String result = "<unknown event>";
            try {
                TextMessage textMessage = (TextMessage) message;
                String xmlContent = textMessage.getText();
                Event ev = (Event)objFactory.unMarshall(xmlContent);		// get the event object
                assertEquals(getTopicname(textMessage), "com.sos.scheduler.engine.Event" );  // Erstmal ist der Klassenname vorangestellt.
                logger.info("CATCH EVENT: " + ev.getName());
                if (ev.getEventOrderSuspended() != null) {
                	scheduler().executeXml("<modify_order job_chain='/" + JOB_CHAIN + "' order='" + ORDER + "' state='success'/>");
                	scheduler().executeXml("<modify_order job_chain='/" + JOB_CHAIN + "' order='" + ORDER + "' suspended='no'/>");
                }
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
    
    private String getTopicname(Message m) throws JMSException {
    	Topic t = (Topic)m.getJMSDestination();
    	return (t.getTopicName()!=null) ? t.getTopicName() : "???";
    }

}
