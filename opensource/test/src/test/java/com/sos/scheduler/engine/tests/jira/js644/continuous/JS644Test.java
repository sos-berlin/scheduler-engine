package com.sos.scheduler.engine.tests.jira.js644.continuous;

import com.google.common.base.Function;
import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.data.folder.AbsolutePath;
import com.sos.scheduler.engine.data.folder.Path;
import com.sos.scheduler.engine.data.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.Gate;
import com.sos.scheduler.engine.main.event.TerminatedEvent;
import com.sos.scheduler.engine.test.Environment;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.junit.SlowTestRule;
import org.apache.log4j.Logger;
import org.junit.ClassRule;
import org.junit.Test;
import org.junit.rules.TestRule;

import java.io.File;

import static com.google.common.collect.Iterables.transform;
import static org.junit.Assert.fail;

/** Der Test lässt einen Auftrag kontinuierlich durch eine Jobkette laufen.
 * Der Thread {@link FilesModifierRunnable} ändert zu zufälligen Zeitpunkten einen Job
 */
public final class JS644Test extends SchedulerTest {
    @ClassRule public static final TestRule slowTestRule = SlowTestRule.singleton;

    private static final Logger logger = Logger.getLogger(JS644Test.class);
    private static final Path jobChainPath = new AbsolutePath("/A");
    private static final ImmutableList<String> jobPaths = ImmutableList.of("a", "b", "c");
    private static final Time orderTimeout = Time.of(60);

    private final Gate<Boolean> threadGate = new Gate<Boolean>();

    @Test public void test() throws InterruptedException {
        controller().startScheduler("-e");
        runModifierThreadAndCheckOrderChanges();
    }

    private void runModifierThreadAndCheckOrderChanges() throws InterruptedException {
        Thread modifierThread = controller().newThread(new FilesModifierRunnable(files()));
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

    private ImmutableList<File> files() {
        ImmutableList.Builder<File> result = ImmutableList.builder();
        result.add(controller().environment().fileFromPath(jobChainPath, ".job_chain.xml"));
        result.addAll(jobFiles());
        return result.build();
    }

    private Iterable<File> jobFiles() {
        final Environment e = controller().environment();
        return transform(jobPaths, new Function<String,File>() {
            @Override public File apply(String o) { return e.fileFromPath(new Path(o), ".job.xml"); }
        });
    }

    @EventHandler public void handleEvent(OrderStateChangedEvent e) throws InterruptedException {
        threadGate.put(true);
    }

    @EventHandler public void handleEvent(TerminatedEvent e) throws InterruptedException {
        threadGate.put(false);
    }
}
