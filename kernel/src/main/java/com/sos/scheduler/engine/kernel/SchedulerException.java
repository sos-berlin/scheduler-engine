package com.sos.scheduler.engine.kernel;


//TODO Meldungscodes erzwingen
public class SchedulerException extends RuntimeException {
    public SchedulerException(String message) { super(message); }
    public SchedulerException(String message, Throwable t) { super(message, t); }
    public SchedulerException(Throwable t) { super(t); }
}
