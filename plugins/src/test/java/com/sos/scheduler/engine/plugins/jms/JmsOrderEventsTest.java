package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.Event;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;
import java.util.Iterator;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

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
    
    private SchedulerObjectFactory objFactory;    
    private int orderFinished = 0;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
    	logger.debug("test started for class " + JmsOrderEventsTest.class.getSimpleName());
	}
    
    
    public JmsOrderEventsTest() throws Exception {
    	super(providerUrl);
    	setMessageListener( new MyListener() );
    }

    @Test
    public void test() throws Exception {
    	try {
    		controller().activateScheduler();
    		objFactory = new SchedulerObjectFactory(scheduler().getHostname(), scheduler().getTcpPort());
    		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
	        controller().scheduler().executeXml( util.addOrder(jobchain, "order1").getCommand() );
	        controller().scheduler().executeXml( util.addOrder(jobchain, "order2").getCommand() );
    		controller().waitForTermination(shortTimeout);
	        assertEvent("EventOrderTouched",2);
	        assertEvent("EventOrderStateChanged",4);
	        assertEvent("EventOrderFinished",2);
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
    
}
