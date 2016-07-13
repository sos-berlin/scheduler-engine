package com.sos.scheduler.engine.cplusplus.runtime;

public class CppException extends RuntimeException {

    public CppException(String message, Throwable t) {
        super(message, t);
    }

    public final String getCode() {
        String m = getCause().getMessage();
        int i = m.indexOf(' ');
        return i <= 0? "UNKNOWN-CPP-ERROR" : m.substring(0, i);
    }
}
