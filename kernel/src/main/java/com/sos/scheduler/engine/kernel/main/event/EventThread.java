package com.sos.scheduler.engine.kernel.main.event;

import org.apache.log4j.Level;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventPredicate;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.ThrowableMailbox;
import java.util.*;
import org.apache.log4j.Logger;
import static com.sos.scheduler.engine.kernel.util.Util.*;


/** Thread zu Verarbeitung von Scheduler-Ereignissen.
 *
 * @author Zschimmer.sos
 */
abstract public class EventThread extends Thread implements EventSubscriber {
    private static final String terminatedEventName = TerminatedEvent.class.getSimpleName();
    private static final Logger logger = Logger.getLogger(EventThread.class);

    private final EventRendezvous rendezvous = new EventRendezvous();
    private final ThrowableMailbox<Throwable> throwableMailbox = new ThrowableMailbox<Throwable>();
    private Collection<EventPredicate> eventPredicates = Collections.singleton(EventPredicate.alwaysTrue);
    private boolean threadIsStarted = true;
    private SchedulerController schedulerController = null;
    private int expectEventCount = 0;


    public EventThread() {
        setName(getClass().getSimpleName());
    }


    public void setEventFilter(EventPredicate... p) {
        setEventFilter(Arrays.asList(p));
    }

    
    public void setEventFilter(Collection<EventPredicate> c) {
        assert c != null;
        eventPredicates = new ArrayList<EventPredicate>(c);
    }


    @Override public void onEvent(Event e) {
        if (e instanceof SchedulerReadyEvent)
            onSchedulerThreadReady((SchedulerReadyEvent)e);
        else
        if (e instanceof TerminatedEvent)
            onSchedulerThreadTerminated((TerminatedEvent)e);
        else
        if (eventMatchesPredicate(e))
            rendezvous.unlockAndCall(e);
    }


    private void onSchedulerThreadReady(SchedulerReadyEvent e) {
        schedulerController = e.getSchedulerController();
        start();  // Thread läuft in run()
        threadIsStarted = true;
    }


    private void onSchedulerThreadTerminated(TerminatedEvent e) {
        try {
            if (threadIsStarted) {
                rendezvous.call(e);     // Immer durchlassen, ohne eventPredicates zu fragen.
                join();
                throwableMailbox.throwUncheckedIfSet();
            }
        }
        catch (InterruptedException x) { throw new SchedulerException(x); }
    }


    private boolean eventMatchesPredicate(Event e) {
        for (EventPredicate p: eventPredicates)  if (p.apply(e))  return true;
        return false;
    }


    @Override public final void run() {
        rendezvous.beginServing();
        
        try {
            runEventThreadAndTerminateScheduler();
        }
//        catch (UnexpectedTerminatedEventException x) {
//            throwableMailbox.setIfFirst(x, Level.DEBUG);
//        }
        catch (Exception x) {
            throwableMailbox.setIfFirst(x);
        }
        catch (Error x) {
            throwableMailbox.setIfFirst(x);
            throw x;
        }
        finally {
            rendezvous.closeServing();
        }
    }


    private void runEventThreadAndTerminateScheduler() throws Exception {
        try {
            runEventThread();
        }
        finally {
            if (!rendezvous.terminatedEventReceived()) {
                schedulerController.terminateScheduler();
                waitForTerminatedEvent(Time.eternal);
            }
        }
    }

    
    abstract protected void runEventThread() throws Exception;

    
    /**
     * @return Das nicht mehr aktuelle Event, weil der Scheduler schon weiterläuft. Die Objekte am Events sind vielleicht nicht mehr gültig.
     */
    public void expectEvent(Time timeout, EventPredicate p) throws InterruptedException {
        expectEvent(new EventExpectation(timeout, p));
    }


    /**
     * @return Das nicht mehr aktuelle Event, weil der Scheduler schon weiterläuft. Die Objekte am Events sind vielleicht nicht mehr gültig.
     */
    public void expectEvent(EventExpectation expect) {
        expectEventCount++;
        EventPredicate p = expect.getPredicate();
        Event e = enterEventHandling(expect.getTimeout());
        try {
            if (!p.apply(e)) throw UnexpectedEventException.of(e, p, expectEventCount);
            if (e instanceof ExceptionEvent) {
                ExceptionEvent xe = (ExceptionEvent)e;
                throw new SchedulerException("Error while polling event: " + xe.exception(), xe.exception() );
            }

            //logger.info("Expected event: " + e);
        }
        finally {
            leaveEventHandling();
        }
    }


//    /**
//     * @return Das nicht mehr aktuelle Event, weil der Scheduler schon weiterläuft. Die Objekte am Events sind vielleicht nicht mehr gültig.
//     */
//    public <T extends Event> T expectEvent(Time timeout, Class<T> clas) throws InterruptedException {
//        Event e = enterEventHandling(timeout);
//        try {
//            return castEvent(clas, e);
//        }
//        finally {
//            leaveEventHandling();
//        }
//    }

    
//    @SuppressWarnings("unchecked")
//    private <T> T castEvent(Class<T> clas, Event e) {
//        if (!clas.isAssignableFrom(e.getClass()))  throw new SchedulerException(clas.getSimpleName() + " expected instead of " + e);
//        return (T)e;
//    }


    public void waitForTerminatedEvent(Time timeout) throws InterruptedException {
        while(!rendezvous.terminatedEventReceived()) {
            Event e = enterEventHandling(timeout);
            if (e == null)  throw new SchedulerException(terminatedEventName + " expected instead of timeout after " + timeout);
            
            try {
//                if (e instanceof SchedulerCloseEvent)  logger.info(e + " ignored");
//                else
                if (!rendezvous.terminatedEventReceived()) {
                    SchedulerException x = new SchedulerException(terminatedEventName + " expected instead of " + e);
                    throwableMailbox.setIfFirst(x);
                }
            }
            finally {
                leaveEventHandling();
            }
        }
    }


    public Event enterEventHandling() {
        return enterEventHandling(Time.eternal);
    }


    public Event enterEventHandling(Time timeout) {
        return rendezvous.enter(timeout);
    }


    public void leaveEventHandling() {
        rendezvous.leave();
    }
}
