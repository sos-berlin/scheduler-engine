package com.sos.scheduler.engine.cplusplus.runtime;

/** Bei Aufruf eines ungültig gewordenen {@link CppProxy}.
 * Der Destruktor des C++-Objekts macht {@link CppProxy} ungültig. */
public class CppProxyInvalidatedException extends RuntimeException {
    public CppProxyInvalidatedException(String message, Throwable cause) {
        super(message, cause);
    }
}
