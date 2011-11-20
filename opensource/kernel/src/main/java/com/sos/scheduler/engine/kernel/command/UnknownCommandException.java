package com.sos.scheduler.engine.kernel.command;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.event.SimpleMessage;


public class UnknownCommandException extends SchedulerException {
    public UnknownCommandException(String commandName) {
        super(new SimpleMessage("SCHEDULER-105", commandName));
    }
}
