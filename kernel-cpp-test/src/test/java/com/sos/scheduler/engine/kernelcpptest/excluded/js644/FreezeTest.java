package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import org.apache.log4j.Logger;
import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.main.SchedulerState;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import java.io.File;
import java.util.Collection;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import org.junit.Test;
import static com.google.common.collect.Iterables.concat;
import static java.util.Arrays.asList;


public class FreezeTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(FreezeTest.class);
    private static final Collection<String> jobFilenames = asList("a.job.xml", "b.job.xml", "c.job.xml");
    private static final Iterable<String> configFilenames = concat(jobFilenames, asList("j.job_chain.xml", "j,1.order.xml"));
    private static final Time orderTimeout = Time.of(10);

    private final BlockingQueue<Boolean> orderFinishedQueue = new ArrayBlockingQueue<Boolean>(1);


    public FreezeTest() {
        super(configFilenames);
    }

    
    @Test public void test() throws Exception {
        strictSubscribeEvents(new MyEventSubscriber());
        startScheduler("-e");
        Thread filesModifierThread = new FilesModifierThread(files(getDirectory(), jobFilenames));
        filesModifierThread.start();
        try {
            for (int i = 0; i < 1000; i++) {
                if (schedulerController.getSchedulerState().equals(SchedulerState.closed))  break;
                Boolean ok = orderFinishedQueue.poll(orderTimeout.getMillis(), TimeUnit.MILLISECONDS);
                assert ok != null: "Auftrag nicht rechtzeitig ausgeführt";
                logger.info("POLL OKAY");
            }
        } finally {
            filesModifierThread.interrupt();
        }
    }


    private static Collection<File> files(File directory, Iterable<String> filenames) {
        ImmutableList.Builder<File> result = new ImmutableList.Builder<File>();
        for (String s: filenames) result.add(new File(directory, s));
        return result.build();
    }


    private class MyEventSubscriber implements EventSubscriber {
        @Override public void onEvent(Event event) throws InterruptedException {
            if (event instanceof OrderStateChangedEvent) {
                OrderStateChangedEvent e = (OrderStateChangedEvent)event;
                if (e.getObject().getState().getString().equals("end")) {
                    orderFinishedQueue.put(true);
                    logger.info("ORDER OKAY");
                }
            } else
            if (event instanceof SchedulerCloseEvent) {
                orderFinishedQueue.put(false);
            }
        }
    }
}
