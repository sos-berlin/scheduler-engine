package com.sos.scheduler.engine.cplusplus.runtime;

import static com.google.common.base.Strings.isNullOrEmpty;

public final class CppProxies {
    //TODO Ist nicht mehr erforderlich, seit CppProxyImpl.cppReference() selbst die Referenz prüft. C++/Java-Generator kann vereinfacht werden.
    public static RuntimeException propagateCppException(Exception x, CppProxy cppProxy) {
        if (x instanceof CppProxyInvalidatedException)
            throw (CppProxyInvalidatedException)x;
        String msg = x.getMessage();
        if (msg != null && msg.startsWith("Z-JAVA-111 "))
            throw new CppProxyInvalidatedException(x.getMessage() +", "+ cppProxy, x);
        throw new CppException(isNullOrEmpty(msg) ? x.toString() : msg, x);
    }

    private CppProxies() {}
}
