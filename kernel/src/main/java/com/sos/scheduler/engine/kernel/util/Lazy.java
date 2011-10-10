package com.sos.scheduler.engine.kernel.util;

public abstract class Lazy<T> {
    private boolean computed = false;
    private T value = null;

    public T apply() {
        if (!computed) {
            value = compute();
            computed = true;
        }
        return value;
    }

    protected abstract T compute();
}
