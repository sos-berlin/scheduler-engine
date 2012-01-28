package com.sos.scheduler.engine.cplusplus.runtime;

import javax.annotation.Nullable;
import java.util.concurrent.atomic.AtomicReference;

public class CppReference<T extends ReleasableCppProxy> {
    private final AtomicReference<T> cppProxy;

    public CppReference(T cppProxy) {
        this.cppProxy = new AtomicReference<T>(cppProxy);
    }

    public final void dispose() {
        if (cppProxy != null) {
            T o = cppProxy.getAndSet(null);
            if (o != null)
                o.Release();
        }
    }

    @Nullable public final T get() {
        return cppProxy.get();
    }

    public static <T extends ReleasableCppProxy> CppReference<T> of(T o) {
        return new CppReference<T>(o);
    }
}
