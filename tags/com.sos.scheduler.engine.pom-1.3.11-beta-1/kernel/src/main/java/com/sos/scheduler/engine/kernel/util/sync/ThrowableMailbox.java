package com.sos.scheduler.engine.kernel.util.sync;

import java.util.concurrent.atomic.AtomicReference;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import static com.sos.scheduler.engine.kernel.util.Util.*;


public class ThrowableMailbox<T extends Throwable> {
    private static final Logger logger = Logger.getLogger(ThrowableMailbox.class);
    
    private final AtomicReference<T> throwableAtom = new AtomicReference<T>();
    
    
    public void setIfFirst(T o) {
        setIfFirst(o, Level.ERROR);
    }


    public void setIfFirst(T o, Level logLevel) {
        boolean isFirst = throwableAtom.compareAndSet(null, o);
        if (!isFirst  &&  logger.isEnabledFor(logLevel)) logger.log(logLevel, "Second exception ignored: " + o, o);
    }


    public void throwIfSet() throws T {
        T o = get();
        if (o != null)  throw o;
    }


    public void throwUncheckedIfSet() {
        T o = get();
        if (o != null)  throwUnchecked(o);
    }


    public T get() {
        return throwableAtom.getAndSet(null);
    }
}
