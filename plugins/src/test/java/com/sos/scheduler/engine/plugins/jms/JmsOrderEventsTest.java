package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.engine.data.EventList;
import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.data.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.codehaus.jackson.map.ObjectMapper;
import org.junit.Test;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;
import java.io.IOException;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import static java.util.Arrays.asList;
import static org.junit.Assert.assertEquals;


public class JmsOrderEventsTest extends JMSConnection {
    /** Maven: mvn test -Dtest=JmsPlugInTest -DargLine=-Djms.providerUrl=tcp://localhost:61616 */
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
    private final static String providerUrl = System.getProperty("jms.providerUrl", ActiveMQConfiguration.vmProviderUrl);
//    private static final String providerUrl = "tcp://w2k3.sos:61616";  // in scheduler.xml einstellen
    private final static Logger logger = Logger.getLogger(JmsOrderEventsTest.class);
    private final CommandBuilder util = new CommandBuilder();
    private final static String jobchain = "jmstest";

    
    // Queue for collecting the fired events in the listener thread
    private final BlockingQueue<String> resultQueue = new ArrayBlockingQueue<String>(50);

    private static final List<String> eventsToListen = asList("OrderTouchedEvent","OrderStateChangedEvent","OrderFinishedEvent");

    private int orderFinished = 0;
    
    public JmsOrderEventsTest() throws Exception {
    	super(providerUrl, eventsToListen);
    	setMessageListener( new JmsListener() );
    }

    @Test
    public void test() throws Exception {
    	try {
    		controller().activateScheduler();
	        controller().scheduler().executeXml( util.addOrder(jobchain, "order1").getCommand() );
	        controller().scheduler().executeXml( util.addOrder(jobchain, "order2").getCommand() );
    		controller().waitForTermination(shortTimeout);
	        assertEvent("OrderTouchedEvent",2);
	        assertEvent("OrderStateChangedEvent",4);
	        assertEvent("OrderFinishedEvent",2);
    	} finally {
    		close();
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
    	logger.debug("ORDERFINISHED: " + o.getId().asString());
    	orderFinished++;
    	if (orderFinished == 2)
    		controller().scheduler().terminate();
    }


    
    private class JmsListener implements javax.jms.MessageListener {

        // This object is needed for serializing and deserializing of the event objects
        private final ObjectMapper mapper;

        public JmsListener() {
            mapper = new ObjectMapper();
            mapper.registerSubtypes( EventList.eventClassList );
        }

        // runs in an own thread
    	@Override
        public void onMessage(Message message) {
            String result = "<unknown event>";
            String jsonContent = null;
            try {
                TextMessage textMessage = (TextMessage) message;
                showMessageHeader(textMessage);
                jsonContent = textMessage.getText();
            	// logger.info("JSON-Content=" + jsonContent);
                Event ev = mapper.readValue(jsonContent, Event.class);
                logger.debug(jsonContent);
                if (ev instanceof OrderStateChangedEvent) {
                    OrderStateChangedEvent ose = (OrderStateChangedEvent)ev;
                }
                if (ev instanceof OrderFinishedEvent) {
                }
                textMessage.acknowledge();
                assertEquals(getTopicname(textMessage), "com.sos.scheduler.engine.Event" );  // Erstmal ist der Klassenname vorangestellt.
                result = ev.getClass().getSimpleName();
            } catch (IOException e1) {
                String msg = "could not deserialize " + jsonContent;
                logger.warn(msg);
            } catch (JMSException e2) {
                String msg = "error getting content from JMS.";
                logger.error(msg);
                throw new SchedulerException(msg,e2);
            } finally {
                try {
					resultQueue.put(result);
				} catch (InterruptedException e) {
					logger.error(e.getMessage());
				}
            }
        }
    }
    
}
