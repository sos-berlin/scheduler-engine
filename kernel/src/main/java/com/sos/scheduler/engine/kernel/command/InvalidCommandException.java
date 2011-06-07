package com.sos.scheduler.engine.kernel.command;

import com.sos.scheduler.engine.kernel.SchedulerException;


public class InvalidCommandException extends SchedulerException {
    public InvalidCommandException(Command c) {
        super("Invalid command " + c);
    }
}

