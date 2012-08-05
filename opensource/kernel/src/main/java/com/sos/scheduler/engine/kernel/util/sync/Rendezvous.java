package com.sos.scheduler.engine.kernel.util.sync;

import com.sos.scheduler.engine.kernel.util.Time;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.SynchronousQueue;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;

import static com.google.common.base.Preconditions.checkNotNull;

/** Zur Rendezvous-Synchronisierung zweier Threads, also zum synchronsierten Aufruf eines Entrys eines anderen Threads.
 * Der rufende Thread ruft call(ARG a) und wartet damit auf den dienenden Thread, bis dieser enter() erreicht.
 * enter() setzt erst fort, wenn ein rufender Thread im call() ist. Dann beginnt das Rendezvous.
 * Der dienende Thread bekommt a während der rufende Thread wartet, bis der dienende Thread leaveException(RESULT r) ruft.
 * r wird dem rufenden Thread zurückgegeben und beide Threads setzen unabhängig fort.
 *
 * @param <ARG> Typ zu Übergabe an den dienenden Thread
 * @param <RESULT> Rückgabe des Rendevous' an den rufenden Thread
 */
public class Rendezvous<ARG,RESULT> {
    private static final Logger logger = Logger.getLogger(Rendezvous.class);

    @Nullable private volatile Thread servingThread = null;
    private volatile boolean inRendezvous = false;
    private final BlockingQueue<ARG> argumentQueue = new SynchronousQueue<ARG>();
    private final BlockingQueue<Return> returnQueue = new SynchronousQueue<Return>();
    private volatile boolean closed = false;

    public final void closeServing() {
        assertIsServingThread();
        try {
            if (argumentQueue.peek() != null)   // Greift nicht immer, dafür müsste die Abfrage synchronisiert werden.
                enter(Time.of(0));

            if (inRendezvous) {
                RendezvousServerClosedException x = new RendezvousServerClosedException();
                logger.error(x, x);
                leaveException(x);
                throw x;
            }
        } finally {
            servingThread = null;
            closed = true;
        }
    }

    public final void beginServing() {
        servingThread = Thread.currentThread();
    }

    private void assertIsNotClosed() {
        if (closed)  throw new RendezvousException("Rendezvous serving is closed");
    }

    public final RESULT call(ARG o) {
        asyncCall(o);
        return awaitResult();
    }

    /** Danach syncResult() aufrufen, bis der Aufruf nicht NULL liefert **/
    public final void asyncCall(ARG o) {
        try {
            assertIsNotClosed();
            argumentQueue.put(o);
        } catch (InterruptedException x) { throw new RuntimeException(x); }
    }

    public final RESULT awaitResult() {
        return checkNotNull(awaitResult(Time.eternal));
    }

    /** Wenn awaitResult() NULL liefert, den Aufruf wiederholen! */
    @Nullable public final RESULT awaitResult(Time t) {
        try {
            Return r = returnQueue.poll(t.value, t.unit);
            if (r == null)
                return null;
            else {
                if  (r.runtimeException != null)  throw r.runtimeException;
                return r.result;
            }
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }

    /** Am Ende immer leave aufrufen! */
    public final ARG enter() {
        return checkNotNull(enter(Time.eternal));
    }

    /** Am Ende immer leave() aufrufen! */
    @Nullable public ARG enter(Time timeout) {
        try {
            assertIsServingThread();
            if (inRendezvous)  throw new IllegalStateException("Already in rendezvous");

            ARG result = timeout == Time.eternal? argumentQueue.take() : argumentQueue.poll(timeout.value, timeout.unit);  // null, if timeout
            if (result != null)  inRendezvous = true;
            return result;
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }

    public void leave(RESULT o) {
        leave(new Return(o));
    }

    public final void leaveException(Throwable t) {
        RuntimeException x = t instanceof RuntimeException? (RuntimeException)t : new RuntimeException(t);
        leave(new Return(x));
    }

    private void leave(Return r) {
        try {
            if (!inRendezvous)  throw new IllegalStateException("Not in rendezvous");
            returnQueue.put(r);
            inRendezvous = false;
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }

    private void assertIsServingThread() {
        if (servingThread == null)  throw new RendezvousException("beginServing() has not been not called");
        if (servingThread != Thread.currentThread())  throw new RendezvousException("Method must be called in serving thread only");
    }

    public final boolean isInRendezvous() {
        return inRendezvous;
    }

    private final class Return {
        private final RESULT result;
        private final RuntimeException runtimeException;
        
        private Return(@Nullable RESULT r, @Nullable RuntimeException x) { result = r; runtimeException = x; }
        private Return(RESULT r) { this(r, null); }
        private Return(RuntimeException x) { this(null, x); }
    }
}
