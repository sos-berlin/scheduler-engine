package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.event.TerminatedEvent;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.ThrowableMailbox;
import java.util.concurrent.atomic.AtomicReference;
import org.apache.log4j.Logger;


/** Der Scheduler in einem eigenen Thread. */
public class SchedulerThread extends Thread implements SchedulerController {
    //TODO Thread auslagern, um nicht zwei Interfaces zu haben. Dann kann getSchedulerState() wieder getState() heißen.
    private static final Logger logger = Logger.getLogger(SchedulerThread.class);

    private final CppScheduler cppScheduler = new CppScheduler();
    private final CoOp coOp = new CoOp();
    private final AtomicReference<Integer> exitCodeAtom = new AtomicReference<Integer>();
    private final ThrowableMailbox<Throwable> throwableMailbox = new ThrowableMailbox<Throwable>();
    private EventSubscriber eventSubscriber = EventSubscriber.empty;
    private String[] arguments = {};
    

    public SchedulerThread() {
        setName("Scheduler");
    }


    @Override public void loadModule() {
        cppScheduler.loadModule();
    }


    @Override public void subscribeEvents(EventSubscriber s) {
        assert s != null;
        eventSubscriber = s;
    }


    @Override public void runScheduler(String... arguments) {
        startScheduler(arguments);
        waitForTermination(terminationTimeout);
    }


    @Override public void startScheduler(String... args) {
        this.arguments = args;
        start();  // Thread läuft in run()
    }


    @Override public Scheduler waitUntilSchedulerIsRunning() {
        try {
            Scheduler result = coOp.waitWhileSchedulerIsStarting();
            throwableMailbox.throwUncheckedIfSet();
            if (result == null) {
                throw new SchedulerException("Scheduler aborted before startup");
            }
            return result;
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }


    @Override public void terminateAndWait() {
        terminateScheduler();
        waitForTermination(Time.eternal);
    }


    @Override public void waitForTermination(Time timeout) {
        try {
            if (timeout == Time.eternal)  join();
            else timeout.unit.timedJoin(this, timeout.value);
            throwableMailbox.throwUncheckedIfSet();
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }


    @Override public void terminateAfterException(Throwable x) {
        throwableMailbox.setIfFirst(x);
        terminateScheduler();
    }


    @Override public void terminateScheduler() {
        coOp.terminate();
    }


//    @Override public Scheduler getScheduler() {
//        Scheduler result = coOp.getScheduler();
//        if (result == null)  throw new NullPointerException(getClass() + " scheduler is not yet running");
//        return result;
//    }


    @Override public int getExitCode() {
        assert !isAlive();
        return exitCodeAtom.get();
    }


    @Override public SchedulerState getSchedulerState() {
        return coOp.getState();
    }


    @Override public void run() {
        int exitCode = -1;
        Throwable t = null;
        try {
            exitCode = cppScheduler.run(arguments, "", new MyMainContext());
            synchronized (exitCodeAtom) { exitCodeAtom.set(exitCode); }
            if (exitCode != 0)  throw new SchedulerException("Scheduler terminated with exit code " + exitCode);
        }
        catch (Exception x) {
            throwableMailbox.setIfFirst(x);
            t = x;
        }
        catch (Error x) {
            throwableMailbox.setIfFirst(x);
            t = x;
            throw x;
        }
        finally {
            coOp.onTerminated();
            strictReportEvent(new TerminatedEvent(exitCode, t));
        }
    }


    private class MyMainContext implements MainContext {
        private MyEventSubscriber myEventSubscriber = new MyEventSubscriber();
        private Scheduler scheduler = null;

        @Override public void setScheduler(Scheduler scheduler) {
            coOp.onSchedulerStarted(scheduler);
            this.scheduler = scheduler;
            strictReportEvent(new SchedulerReadyEvent(SchedulerThread.this));
        }

        @Override public void onSchedulerActivated() {
            scheduler.getEventSubsystem().subscribe(myEventSubscriber);
        }
        
        class MyEventSubscriber implements EventSubscriber {
            @Override public void onEvent(Event e) throws Exception {
                if (e instanceof SchedulerCloseEvent)
                    coOp.onSchedulerClosed();
                eventSubscriber.onEvent(e);
            }
        }
    }

    
    private void strictReportEvent(Event e) {
        try { 
            eventSubscriber.onEvent(e);
        }
        catch (Throwable x) {
            //boolean debugOnly = e instanceof TerminatedEvent  &&  x instanceof UnexpectedTerminatedEventException;
            throwableMailbox.setIfFirst(x); //, debugOnly? Level.DEBUG : Level.ERROR);
        }
    }
}
