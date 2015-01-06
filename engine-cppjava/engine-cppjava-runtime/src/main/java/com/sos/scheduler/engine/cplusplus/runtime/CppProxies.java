package com.sos.scheduler.engine.cplusplus.runtime;

public final class CppProxies {
    //TODO Ist nicht mehr erforderlich, seit CppProxyImpl.cppReference() selbst die Referenz prüft. C++/Java-Generator kann vereinfacht werden.
    public static RuntimeException propagateCppException(Exception x, CppProxy cppProxy) {
        if (x instanceof CppProxyInvalidatedException)
            throw (CppProxyInvalidatedException)x;
        String msg = x.getMessage();
        if (msg.startsWith("Z-JAVA-111 "))
            throw new CppProxyInvalidatedException(x.getMessage() +", "+ cppProxy, x);
        throw new CppException(x);
    }

    private CppProxies() {}
}
