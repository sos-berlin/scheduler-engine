package com.sos.scheduler.engine.kernel.util.sync;

import static com.google.common.base.Objects.firstNonNull;
import static com.google.common.base.Throwables.propagate;

import java.util.concurrent.atomic.AtomicReference;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;

public class ThrowableMailbox<T extends Throwable> {
    private static final Logger logger = Logger.getLogger(ThrowableMailbox.class);
    
    private final AtomicReference<T> throwableAtom = new AtomicReference<T>();
    
    public final void setIfFirst(T o) {
        setIfFirst(o, Level.ERROR);
    }

    public final void setIfFirst(T o, Level logLevel) {
        boolean isFirst = throwableAtom.compareAndSet(null, o);
        if (!isFirst  &&  logger.isEnabledFor(logLevel)) logger.log(logLevel, "Second exception ignored: " + o, o);
    }

//    public final void throwIfSet() throws T {
//        T o = fetch();
//        if (o != null)  throw o;
//    }

    public final void throwUncheckedIfSet() {
        T throwable = fetch();
        if (throwable != null) throw propagate(throwable);
    }

    private T fetch() {
        return throwableAtom.getAndSet(null);
    }

    @Override public String toString() {
        return ThrowableMailbox.class.getSimpleName() +"("+ firstNonNull(throwableAtom.get(), "") +")";
    }
}
