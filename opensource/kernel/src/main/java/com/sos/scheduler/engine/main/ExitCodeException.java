package com.sos.scheduler.engine.main;

public class ExitCodeException extends RuntimeException {
    public ExitCodeException(int exitCode) {
        super("Terminated with exit code " + exitCode);
    }
}
