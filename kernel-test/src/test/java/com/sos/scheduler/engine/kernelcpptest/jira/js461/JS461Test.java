package com.sos.scheduler.engine.kernelcpptest.jira.js461;

import static org.junit.Assert.assertEquals;

import java.util.Iterator;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import org.junit.BeforeClass;
import org.junit.Ignore;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderResumedEvent;
import com.sos.scheduler.engine.kernel.order.OrderSuspendedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.model.SchedulerObjectFactory;

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
public class JS461Test extends SchedulerTest {

	private final String ORDER = "js-461-order";
	private final String JOB_CHAIN = "js-461";
	
    private static final Time schedulerTimeout = Time.of(10);
    private static Logger logger;
    
    // Queue for collecting the fired events in the listener thread
    private final BlockingQueue<String> resultQueue = new ArrayBlockingQueue<String>(50);
    private final SchedulerObjectFactory objFactory;
    
    @BeforeClass
    public static void setUpBeforeClass () throws Exception {
		logger = LoggerFactory.getLogger(JS461Test.class);
	}
    
    public JS461Test() throws Exception {
        controller().setTerminateOnError(false);
    	
		objFactory = new SchedulerObjectFactory("localhost", 4444);
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);
    }
    
    /**
     * \brief Test ausführen
     * \detail
     * Es werden 2 OrderFinishedEvents erwartet (und sonst nichts)
     *
     * @throws Exception
     */
    @Ignore 
    public void test() throws Exception {
        controller().subscribeEvents(new MyEventSubscriber());
        controller().runScheduler(schedulerTimeout, "-e");
        assertEvent("OrderSuspendedEvent",1);										// one order has to end with 'success'
        assertEvent("OrderResumedEvent",1);										// one order has to end with 'success'
        assertEvent("OrderFinishedEvent",1);										// one order has to end with 'success'
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
    
    private final class MyEventSubscriber implements EventSubscriber {

		@Override
		public void onEvent(Event event)
				throws Exception {
			
			String result = "<ignored>";
			try {
	            if (event instanceof OrderResumedEvent) {
	            	result = "OrderResumedEvent";
				}
	            if (event instanceof OrderFinishedEvent) {
	            	result = "OrderFinishedEvent";
				}
		        if (event instanceof OrderSuspendedEvent) {
		        	result = "OrderSuspendedEvent";
		        	scheduler().executeXml("<modify_order job_chain='/" + JOB_CHAIN + "' order='" + ORDER + "' state='success'/>");
		    		scheduler().executeXml("<modify_order job_chain='/" + JOB_CHAIN + "' order='" + ORDER + "' suspended='no'/>");
				}
			}
            finally {
                try {
					if (!result.equals("<ignored>")) resultQueue.put(result);
				} catch (InterruptedException e) {
					logger.error(e.getMessage());
				}
            }
	    }
    }

}
