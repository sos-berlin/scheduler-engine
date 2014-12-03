package com.sos.scheduler.engine.main.event;

public class ExceptionEvent extends MainEvent {
    private final Throwable throwable;

    public ExceptionEvent(Throwable x) {
        throwable = x;
    }

    public final Throwable getThrowable() {
        return throwable;
    }

    @Override public final String toString() {
        return super.toString() + " " + throwable;
    }
}
