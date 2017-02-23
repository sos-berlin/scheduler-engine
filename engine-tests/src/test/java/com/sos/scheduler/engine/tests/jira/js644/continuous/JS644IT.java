package com.sos.scheduler.engine.tests.jira.js644.continuous;

import com.google.common.base.Function;
import com.google.common.collect.ImmutableList;
import com.sos.jobscheduler.data.event.Event;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.common.sync.Gate;
import com.sos.scheduler.engine.data.job.JobPath;
import com.sos.scheduler.engine.data.jobchain.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderNodeChanged;
import com.sos.scheduler.engine.data.scheduler.SchedulerTerminatedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.TestEnvironment;
import com.sos.scheduler.engine.test.junit.SlowTestRule;
import java.io.File;
import java.time.Duration;
import org.junit.ClassRule;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import static com.google.common.collect.Iterables.transform;
import static java.util.Arrays.asList;
import static org.junit.Assert.fail;

/** Der Test lässt einen Auftrag kontinuierlich durch eine Jobkette laufen.
 * Der Thread {@link FilesModifierRunnable} ändert zu zufälligen Zeitpunkten einen Job
 */
@Ignore //slowTestRule greift nicht mehr - 2012-09-29
public final class JS644IT extends SchedulerTest {
    @ClassRule public static final TestRule slowTestRule = SlowTestRule.singleton;

    private static final Logger logger = LoggerFactory.getLogger(JS644IT.class);
    private static final JobChainPath jobChainPath = new JobChainPath("/A");
    private static final ImmutableList<String> jobPaths = ImmutableList.of("/a", "/b", "/c");
    private static final Duration orderTimeout = Duration.ofSeconds(60);

    private final Gate<Boolean> threadGate = new Gate<Boolean>();

    @Test public void test() throws InterruptedException {
        controller().activateScheduler(asList("-e"));
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
        result.add(controller().environment().fileFromPath(jobChainPath));
        result.addAll(jobFiles());
        return result.build();
    }

    private Iterable<File> jobFiles() {
        final TestEnvironment e = controller().environment();
        return transform(jobPaths, new Function<String,File>() {
            @Override public File apply(String o) { return e.fileFromPath(new JobPath(o)); }
        });
    }

    @EventHandler public void handleEvent(KeyedEvent<Event> e) throws InterruptedException {
        if (OrderNodeChanged.class.isAssignableFrom(e.event().getClass())) {
            threadGate.put(true);
        } else
        if (SchedulerTerminatedEvent.class.isAssignableFrom(e.event().getClass())) {
            threadGate.put(false);
        }
    }
}
