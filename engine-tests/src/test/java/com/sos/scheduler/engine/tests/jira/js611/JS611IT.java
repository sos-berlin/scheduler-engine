package com.sos.scheduler.engine.tests.jira.js611;

import com.google.common.base.Predicate;
import com.google.common.base.Splitter;
import com.google.common.collect.Iterables;
import com.google.common.io.Files;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.order.OrderFinished;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import java.io.File;
import java.io.IOException;
import org.junit.Test;
import static com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.logFileEncoding;

/** Test, ob Scheduler Log-Dateien vorübergehend schließt, wenn mehr Logs als festgelegt geschrieben werden.
 * @see <a href="http://www.sos-berlin.com/jira/browse/JS-611">JS-611</a> */
public final class JS611IT extends SchedulerTest {
	private static final int maxOrderCount = 10;

    private int finishedOrderCount = 0;

    @Test public void test() throws Exception {
        controller().activateScheduler();
        scheduler().executeXml("<scheduler_log.log_categories.set category='JS-611'/>");
        addOrders(maxOrderCount);
        controller().waitForTermination();
        checkSchedulerLog(controller().environment().schedulerLog());
    }

    private void addOrders(int n) {
    	for (int i = 0; i < n; i++)
    		scheduler().executeXml("<add_order job_chain='/A' id='" + i + "'/>");
    }

    private static void checkSchedulerLog(File schedulerLogFile) throws IOException {
        String schedulerLog = Files.toString(schedulerLogFile, logFileEncoding);
        Iterable<String> js611Lines = Iterables.filter(Splitter.on("\n").split(schedulerLog), new Predicate<String>() {
            public boolean apply(String o) {
                return o.contains(" {JS-611}");
            }
        });
        //TODO  Jetzt die close_file-Zeilen pro Auftrag zählen. Sollte ab 51 Logs mehr als eine pro Auftrag sein. Aber 25 Tasks brauchen mehrere Gigabyte Adressraum ...
    }

    @EventHandler public void handle(KeyedEvent<OrderFinished> g) {
        finishedOrderCount += 1;
        if (finishedOrderCount == maxOrderCount)
            controller().terminateScheduler();
    }
}
