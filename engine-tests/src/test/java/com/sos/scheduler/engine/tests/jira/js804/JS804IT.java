package com.sos.scheduler.engine.tests.jira.js804;

import com.sos.scheduler.engine.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.job.TaskEnded;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.configuration.TestConfigurationBuilder;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import com.sos.scheduler.engine.test.util.What;
import java.io.StringWriter;
import org.junit.Test;
import org.w3c.dom.Document;
import static com.sos.scheduler.engine.common.xml.CppXmlUtils.loadXml;
import static com.sos.scheduler.engine.common.xml.CppXmlUtils.stringXPath;
import static com.sos.scheduler.engine.common.xml.CppXmlUtils.writeXmlTo;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * This test demonstrates the result attribute <i>setback</i> of the <i>show_calendar</i> command.
 * The order <i>js-804</i> goes to the setback NodeId immediately after starting. The result of
 * <i>show_calendar</i> must contain <i>setback='true'</i> for it.
 * In opposite is order <i>js-804-1</i>. It is a simple order (scheduled with the start of JS) with
 * no setback.  The result of <i>show_calendar</i> must not contain the <i>setback</i> because <i>false</i>
 * is the default for this attribute.
 */
public final class JS804IT extends SchedulerTest {

	private static final int ONE_DAY = 86400;
	private static final String order_setback = "js804";		// to be started via the test
	private static final String order_simple = "js804-1";		// not started but scheduled

	private Document showCalendarAnswer;
	private final CommandBuilder util = new CommandBuilder();
	private boolean result_setback = false;
	private boolean result_simple = true;

    public JS804IT() {
        super(new TestConfigurationBuilder(JS804IT.class).terminateOnError(false).build());
    }

	@Test
	public void testSetback() {
		controller().activateScheduler();
		controller().scheduler().executeXml( util.modifyOrder(order_setback).getCommand() );
		controller().waitForTermination();
		assertTrue("order " + order_setback + " is not in setback", result_setback);
		assertFalse("order " + order_simple + " is in setback", result_simple);
	}

	@EventHandler
    public void handleEvent(KeyedEvent<TaskEnded> e) throws InterruptedException {
        Thread.sleep(2000);			// wait until setback is active
        showCalendar();
        result_setback = isSetback(order_setback);
        result_simple = isSetback(order_simple);
        controller().terminateScheduler();
	}

    private void showCalendar() {
    	showCalendarAnswer = loadXml(scheduler().executeXml(util.showCalendar(ONE_DAY, What.orders).getCommand()));
        StringWriter sw = new StringWriter();
        boolean notIndent = false;
        writeXmlTo(showCalendarAnswer.getFirstChild(), sw, notIndent);
    }

    private boolean isSetback(String order) {
        return stringXPath(showCalendarAnswer, "/spooler/answer/calendar/at[@order='" + order + "']/@setback").equals("true");
    }
}
