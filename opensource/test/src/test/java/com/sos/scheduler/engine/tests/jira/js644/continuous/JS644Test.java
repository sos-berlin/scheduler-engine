package com.sos.scheduler.engine.tests.jira.js644.continuous;

import static com.google.common.collect.Iterables.transform;
import static java.util.Arrays.asList;
import static org.junit.Assert.fail;

import java.io.File;
import java.util.List;

import org.apache.log4j.Logger;
import org.junit.ClassRule;
import org.junit.Test;
import org.junit.rules.TestRule;

import com.google.common.base.Function;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.folder.Path;
import com.sos.scheduler.engine.main.event.TerminatedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.junit.SlowTestRule;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.Gate;

/** Der Test lässt einen Auftrag kontinuierlich durch eine Jobkette laufen.
 * Der Thread {@link FilesModifierRunnable} ändert zu zufälligen Zeitpunkten einen Job
 */
public final class JS644Test extends SchedulerTest {
    @ClassRule public static final TestRule slowTestRule = SlowTestRule.singleton;

    private static final Logger logger = Logger.getLogger(JS644Test.class);
    private static final List<String> jobPaths = asList("a", "b", "c");
    private static final Time orderTimeout = Time.of(60);

    private final Gate<Boolean> threadGate = new Gate<Boolean>();

    @Test public void test() throws InterruptedException {
        controller().startScheduler("-e");
        runModifierThreadAndCheckOrderChanges();
    }

    private void runModifierThreadAndCheckOrderChanges() throws InterruptedException {
        Thread modifierThread = controller().newThread(new FilesModifierRunnable(jobFiles()));
        modifierThread.start();
        try {
            runUntilJobChainOrSchedulerStops();
        } finally {
            modifierThread.interrupt();
            modifierThread.join();
        }
    }

    private void runUntilJobChainOrSchedulerStops() throws InterruptedException {
        while (waitForChangedOrder()) {}
    }

    private boolean waitForChangedOrder() throws InterruptedException {
        Boolean ok = threadGate.poll(orderTimeout);
        if (ok == null) fail("An order has not been processed in time");
        if (ok) logger.debug("An order has been finished");
        return ok;
    }

    private Iterable<File> jobFiles() {
        return transform(jobPaths, new Function<String,File>() {
            @Override public File apply(String o) { return controller().environment().fileFromPath(new Path(o), ".job.xml"); }
        });
    }

    @EventHandler public void handleEvent(OrderStateChangedEvent e) throws InterruptedException {
        threadGate.put(true);
    }

    @EventHandler public void handleEvent(TerminatedEvent e) throws InterruptedException {
        threadGate.put(false);
    }
}
