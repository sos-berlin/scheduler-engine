package com.sos.scheduler.engine.cplusplus.runtime;

public final class CppProxies {
    public static RuntimeException propagateCppException(Exception x, CppProxy cppProxy) {
        String msg = x.getMessage();
        if (msg.startsWith("Z-JAVA-111 "))
            throw new CppProxyInvalidatedException(x.getMessage(), x);
        throw new CppException(x);
    }

    private CppProxies() {}
}
