package com.sos.scheduler.engine.kernel.main.event;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.EventPredicate;
import com.sos.scheduler.engine.eventbus.EventSubscription;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.ThrowableMailbox;

/** Thread zu Verarbeitung von Scheduler-Ereignissen.
 *
 * @author Zschimmer.sos
 */
public abstract class EventThread extends Thread implements EventSubscription {
    private static final String terminatedEventName = TerminatedEvent.class.getSimpleName();

    private final EventRendezvous rendezvous = new EventRendezvous();
    private final ThrowableMailbox<Throwable> throwableMailbox = new ThrowableMailbox<Throwable>();
    private Collection<EventPredicate> eventPredicates = Collections.singleton(EventPredicate.alwaysTrue);
    private boolean threadIsStarted = true;
    private final SchedulerController schedulerController;
    private int expectEventCount = 0;

    protected EventThread(SchedulerController schedulerController) {
        this.schedulerController = schedulerController;
        setName(getClass().getSimpleName());
    }

    @Override public Class<? extends Event> getEventClass() {
        return Event.class;
    }

    public final void setEventFilter(EventPredicate... p) {
        setEventFilter(Arrays.asList(p));
    }

    public final void setEventFilter(Collection<EventPredicate> c) {
        assert c != null;
        eventPredicates = new ArrayList<EventPredicate>(c);
    }

    @Override public final void handleEvent(Event e) {
        if (e instanceof SchedulerReadyEvent)
            onSchedulerThreadReady((SchedulerReadyEvent)e);
        else
        if (e instanceof TerminatedEvent)
            onSchedulerThreadTerminated((TerminatedEvent)e);
        else
        if (eventMatchesPredicate(e)) {
            if (isAlive())
                rendezvous.unlockAndCall(e);
        }
    }

    private void onSchedulerThreadReady(SchedulerReadyEvent e) {
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

    protected abstract void runEventThread() throws Exception;

    /**
     * @return Das nicht mehr aktuelle Event, weil der Scheduler schon weiterläuft. Die Objekte am Events sind vielleicht nicht mehr gültig.
     */
    public final void expectEvent(Time timeout, EventPredicate p) {
        expectEvent(new EventExpectation(timeout, p));
    }

    /**
     * @return Das nicht mehr aktuelle Event, weil der Scheduler schon weiterläuft. Die Objekte am Events sind vielleicht nicht mehr gültig.
     */
    public final void expectEvent(EventExpectation expect) {
        expectEventCount++;
        EventPredicate p = expect.getPredicate();
        Event e = enterEventHandling(expect.getTimeout());
        try {
            if (!p.apply(e)) throw UnexpectedEventException.of(e, p, expectEventCount);
            if (e instanceof ExceptionEvent) {
                ExceptionEvent xe = (ExceptionEvent)e;
                Throwable t = xe.getThrowable();
                throw new SchedulerException("Error while polling event: "+t, t);
            }
        }
        finally {
            leaveEventHandling();
        }
    }

    public final void waitForTerminatedEvent(Time timeout) {
        while(!rendezvous.terminatedEventReceived()) {
            Event e = enterEventHandling(timeout);
            if (e == null)  throw new SchedulerException(terminatedEventName + " expected instead of timeout after " + timeout);
            
            try {
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

    public final Event enterEventHandling() {
        return enterEventHandling(Time.eternal);
    }

    public final Event enterEventHandling(Time timeout) {
        return rendezvous.enter(timeout);
    }

    public final void leaveEventHandling() {
        rendezvous.leave();
    }
}
