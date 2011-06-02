package com.sos.scheduler.engine.kernel.command;


public interface CommandExecutor {
    Class<? extends Command> getCommandClass();
    Result execute(Command c);
}
