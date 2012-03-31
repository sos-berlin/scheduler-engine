package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.Event;
import org.apache.log4j.Logger;
import org.junit.Test;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import static java.util.Arrays.asList;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;


public class JmsEventFilterTest extends JMSConnection {
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
    /** Maven: mvn test -Dtest=JmsPlugInTest -DargLine=-Djms.providerUrl=tcp://localhost:61616 */
    private final static String providerUrl = System.getProperty("jms.providerUrl", ActiveMQConfiguration.vmProviderUrl);
//  private static final String providerUrl = "tcp://w2k3.sos:61616";  // in scheduler.xml einstellen
    private static final Logger logger = Logger.getLogger(JmsEventFilterTest.class);
    
    private final CommandBuilder util = new CommandBuilder();

    private static final List<String> eventsToListen = asList("EventOrderTouched");
    private final static String jobchain = "jmstest";
    private int orderFinished = 0;
    
    // Queue for collecting the fired eventsToListen in the listener thread
    private final BlockingQueue<String> resultQueue = new ArrayBlockingQueue<String>(50);
    
    private SchedulerObjectFactory objFactory;
    
    public JmsEventFilterTest() throws Exception {
    	
    	super(providerUrl,eventsToListen);
    	setMessageListener( new MyListener() );

    }

    @Test
    public void test() throws Exception {
    	try {
//	        controller().activateScheduler("-e -log-level=debug","-log=" + FileUtils.getLocalFile(this.getClass(), "scheduler.log"));
	        controller().activateScheduler();
			objFactory = new SchedulerObjectFactory(scheduler().getHostname(), scheduler().getTcpPort());
			objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
	        controller().scheduler().executeXml( util.addOrder(jobchain, "order1").getCommand() );
	        controller().scheduler().executeXml( util.addOrder(jobchain, "order2").getCommand() );
	        controller().waitForTermination(shortTimeout);
	        assertEquals("two eventsToListen of " + eventsToListen.get(0) + " expected",2,resultQueue.size());
	        assertTrue("'order1' is not in result queue",resultQueue.contains("order1"));
	        assertTrue("'order2' is not in result queue",resultQueue.contains("order2"));
		} finally {
			close();
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
                	throw new SchedulerException(ev.getName() + " not expected - expected");
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
    
}
