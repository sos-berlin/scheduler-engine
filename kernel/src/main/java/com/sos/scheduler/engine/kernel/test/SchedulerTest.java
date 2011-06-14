package com.sos.scheduler.engine.kernel.test;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.main.SchedulerThread;
import com.sos.scheduler.engine.kernel.util.Time;
import java.util.ArrayList;
import java.util.Arrays;
import org.apache.log4j.Logger;
import org.junit.After;


public abstract class SchedulerTest {
    public static final Time shortTimeout = Time.of(10);
    private static final Logger logger = Logger.getLogger(SchedulerTest.class);

    protected final SchedulerController schedulerController;
    protected Scheduler scheduler = null;
    private final Environment env = new Environment(this);


    public SchedulerTest() {
        schedulerController = new SchedulerThread();
    }


    public final void strictSubscribeEvents() {
        strictSubscribeEvents(EventSubscriber.empty);
    }


    public final void strictSubscribeEvents(EventSubscriber s) {
        schedulerController.subscribeEvents(new StrictEventSubscriber(s));
    }


    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }


//    public void runScheduler(String... args) {
//        runScheduler(Time.eternal, args);
//    }


    public final void startScheduler(String... args) {
        ArrayList<String> allArgs = new ArrayList<String>();
        allArgs.addAll(Arrays.asList(env.standardArgs()));
        allArgs.addAll(Arrays.asList(args));
        schedulerController.startScheduler(allArgs.toArray(new String[0]));
    }


    public final void waitUntilSchedulerIsRunning() {
        scheduler = schedulerController.waitUntilSchedulerIsRunning();
    }


    public final Scheduler getScheduler() {
        if (scheduler == null)
            waitUntilSchedulerIsRunning();
        assert scheduler != null;
        return scheduler;
    }


    public final void waitForTermination(Time timeout) {
        schedulerController.waitForTermination(timeout);
    }


    @After public final void terminateAndCleanUp() throws Throwable {
        try {
            schedulerController.terminateAndWait();
        }
        catch (Throwable x) {
            logger.error(SchedulerTest.class.getName() + " @After: " + x, x);
            throw x;
        }
        finally {
            env.cleanUp();
        }
    }
}
