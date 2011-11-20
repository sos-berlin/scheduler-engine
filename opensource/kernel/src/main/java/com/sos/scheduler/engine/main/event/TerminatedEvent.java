package com.sos.scheduler.engine.main.event;

public final class TerminatedEvent extends MainEvent {
    private final int exitCode;
    private final Throwable throwable;

    /** @param x can be null */
    public TerminatedEvent(int exitCode, Throwable x) {
        this.exitCode = exitCode;
        this.throwable = x;
    }

    public int getExitCode() {
        return exitCode;
    }

    /** @return null, when scheduler terminated without exception */
    public Throwable getThrowable() {
        return throwable;
    }

    @Override public String toString() {
        StringBuilder result = new StringBuilder(200);

        result.append(super.toString());
        if (exitCode != 0)  result.append(" exitCode=").append(exitCode);
        if (throwable != null)  result.append(" ").append(throwable.toString());

        return result.toString();
    }
}
