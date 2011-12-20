package com.sos.scheduler.engine.tests.jira.js804;

import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.stringXPath;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.writeXmlTo;
import static org.junit.Assert.*;

import java.io.IOException;
import java.io.StringWriter;

import org.apache.log4j.Logger;
import org.junit.Test;
import org.w3c.dom.Document;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.JSTestUtils;
import com.sos.scheduler.engine.test.util.What;

/**
 * This test demonstrates the result attribute <i>setback</i> of the <i>show_calendar</i> command.
 * The order <i>js-804</i> goes to the setback state immediately after starting. The result of
 * <i>show_calendar</i> must contain <i>setback='true'</i> for it. 
 * In opposite is order <i>js-804-1</i>. It is a simple order (scheduled with the start of JS) with
 * no setback.  The result of <i>show_calendar</i> must not contain the <i>setback</i> because <i>false</i>
 * is the default for this attribute. 
 * 
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 20.12.2011 12:32:07
 *
 */
public class JS804Test extends SchedulerTest {

	private static final Logger logger = Logger.getLogger(JS804Test.class);
	private final static int ONE_DAY = 86400;
	
	private final String order_setback = "js804";		// to be started via the test
	private final String order_simple = "js804-1";		// not started but scheduled
	private Document showCalendarAnswer;	
	private JSTestUtils util = JSTestUtils.getInstance();
	private boolean result_setback = false;
	private boolean result_simple = true;
	
	@Test
	public void TestSetback() throws InterruptedException, IOException {
		controller().setTerminateOnError(false);
		controller().startScheduler();
		controller().scheduler().executeXml( util.buildCommandModifyOrder(order_setback) );
		controller().tryWaitForTermination(shortTimeout);
		assertTrue("order " + order_setback + " is not in setback",result_setback);
		assertFalse("order " + order_simple + " is in setback",result_simple);
	}

	@EventHandler
	public void handleTaskEnded(TaskEndedEvent e) throws IOException, InterruptedException {
		Thread.sleep(2000);			// wait until setback is active
		showCalendar();
		result_setback = isSetback(order_setback);
		result_simple = isSetback(order_simple);
		controller().terminateScheduler();
	}

    private void showCalendar() {
    	showCalendarAnswer = loadXml(scheduler().executeXml(util.buildCommandShowCalendar(ONE_DAY, What.orders)));
        StringWriter sw = new StringWriter();
        writeXmlTo(showCalendarAnswer.getFirstChild(),sw);
        logger.debug(sw.toString());
    }

    private boolean isSetback(String order) {
        return stringXPath(showCalendarAnswer, "/spooler/answer/calendar/at[@order='" + order + "']/@setback").equals("true");
    }

}
