package com.sos.scheduler.engine.cplusplus.runtime;

import static com.google.common.base.Throwables.propagate;

public final class CppProxies {
    public static RuntimeException propagateCppException(Exception x, CppProxy cppProxy) {
        String msg = x.getMessage();
        if (msg.startsWith("Z-JAVA-111 "))
            throw new CppProxyInvalidatedException(x.getMessage(), x);
        throw propagate(x);
    }

    private CppProxies() {}
}
