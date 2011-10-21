package com.sos.scheduler.engine.kernel.util;

/** Thread-sichere verzögerte Berechnung (oder Initialisierung) eines Werts. */
public abstract class Lazy<T> {
    private volatile boolean computed = false;
    private volatile T value = null;

    public final T apply() {
        if (!computed) {
            synchronized (this) {
                if (!computed) {
                    value = compute();
                    computed = true;
                }
            }
        }
        return value;
    }

    protected abstract T compute();
}
