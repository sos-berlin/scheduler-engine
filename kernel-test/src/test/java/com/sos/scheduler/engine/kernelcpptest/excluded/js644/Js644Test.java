package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import static com.google.common.base.Throwables.propagate;
import static com.google.common.collect.Iterables.transform;
import static com.sos.scheduler.engine.kernel.util.Util.sleepUntilInterrupted;
import static java.util.Arrays.asList;
import static org.junit.Assert.fail;

import java.io.File;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

import org.apache.log4j.Logger;
import org.junit.ClassRule;
import org.junit.Test;
import org.junit.rules.TestRule;

import com.google.common.base.Function;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.event.TerminatedEvent;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.test.junit.SlowTestRule;
import com.sos.scheduler.engine.kernel.util.Time;

public final class Js644Test extends SchedulerTest {
    @ClassRule public static final TestRule slowTestRule = SlowTestRule.singleton;
    private static final Logger logger = Logger.getLogger(Js644Test.class);
    static final List<String> jobPaths = asList("a", "b", "c");
    private static final Time orderTimeout = Time.of(10);

    private final BlockingQueue<Boolean> eventReceivedQueue = new ArrayBlockingQueue<Boolean>(1);


    @Test public void test() throws InterruptedException {
        controller().strictSubscribeEvents(new MyEventSubscriber());
        controller().startScheduler("-e");
        Thread modifierThread = new Thread(new ControllerRunnable(new FilesModifierRunnable(jobFiles())));
        modifierThread.start();
        try {
            while(true) {
                // Der Test läuft ewig bis ein Job stehen bleibt und ein Auftrag nicht rechtzeitig verarbeitet worden ist.
                Boolean ok = eventReceivedQueue.poll(orderTimeout.getMillis(), TimeUnit.MILLISECONDS);
                if (ok == null) fail("An order has not been processed in time");
                else if (!ok)  break;
                logger.debug("An order has been finished");
            }
        } finally {
            modifierThread.interrupt();
            modifierThread.join();
        }
    }

    private Iterable<File> jobFiles() {
        return transform(jobPaths, new Function<String,File>() {
            @Override public File apply(String o) { return new File(controller().environment().configDirectory(), o + ".job.xml"); }
        });
    }

    private final class MyEventSubscriber implements EventSubscriber {
        @Override public void onEvent(Event event) throws InterruptedException {
            logger.debug(event);
            if (event instanceof OrderStateChangedEvent)
                eventReceivedQueue.put(true);
            else
            if (event instanceof TerminatedEvent)
                eventReceivedQueue.put(false);
        }
    }

    private final class ControllerRunnable implements Runnable {
        private final Runnable runnable;

        ControllerRunnable(Runnable runnable) {
            this.runnable = runnable;
        }

        @Override public void run() {
            try {
                runnable.run();
            } catch (Throwable t) {
                controller().terminateAfterException(t);
                throw propagate(t);
            }
        }
    }

    private static final class FilesModifierRunnable implements Runnable {
        private final FilesModifier filesModifier;

        FilesModifierRunnable(Iterable<File> files) {
            filesModifier = new FilesModifier(files);
        }

        @Override public void run() {
            int pause = 4000 / filesModifier.fileCount() + 1;   // Mindestens 2s Pause, damit Scheduler Datei als stabil ansieht.
            while(true) {
                filesModifier.modifyNext();
                boolean interrupted = sleepUntilInterrupted(pause);
                if (interrupted) break;
            }
        }
    }
}
