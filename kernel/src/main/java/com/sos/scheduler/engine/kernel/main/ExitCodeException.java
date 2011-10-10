package com.sos.scheduler.engine.kernel.main;

public class ExitCodeException extends RuntimeException {
    public ExitCodeException(int exitCode) {
        super("Terminated with exit code " + exitCode);
    }
}
