package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import static com.google.common.base.Throwables.propagate;
import static com.google.common.collect.Iterables.transform;
import static com.sos.scheduler.engine.kernelcpptest.excluded.js644.Js644Test.E.orderChanged;
import static com.sos.scheduler.engine.kernelcpptest.excluded.js644.Js644Test.E.terminated;
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
    private static final List<String> jobPaths = asList("a", "b", "c");
    private static final Time orderTimeout = Time.of(60);

    enum E { orderChanged, terminated };
    private final BlockingQueue<E> eventReceivedQueue = new ArrayBlockingQueue<E>(1);

    @Test public void test() throws InterruptedException {
        controller().strictSubscribeEvents(new MyEventSubscriber());
        controller().startScheduler("-e");
        runModifierThreadAndCheckOrderChanges();
    }

    private void runModifierThreadAndCheckOrderChanges() throws InterruptedException {
        Thread modifierThread = new Thread(new ControllerRunnable(new FilesModifierRunnable(jobFiles())));
        modifierThread.setName("job.xml modifier");
        modifierThread.start();
        try {
            while(true) { // Der Test läuft ewig bis ein Job stehen bleibt und ein Auftrag nicht rechtzeitig verarbeitet worden ist.
                E e = waitForChangedOrder();
                if (e == terminated)  break;
            }
        } finally {
            modifierThread.interrupt();
            modifierThread.join();
        }
    }

    private E waitForChangedOrder() throws InterruptedException {
        E e = eventReceivedQueue.poll(orderTimeout.getMillis(), TimeUnit.MILLISECONDS);
        if (e == null) fail("An order has not been processed in time");
        if (e == orderChanged) logger.debug("An order has been finished");
        return e;
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
                eventReceivedQueue.put(orderChanged);
            else
            if (event instanceof TerminatedEvent)
                eventReceivedQueue.put(terminated);
        }
    }

    final class ControllerRunnable implements Runnable {
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
}
