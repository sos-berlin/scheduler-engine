package com.sos.scheduler.engine.kernel.command;


public interface CommandExecutor extends CommandHandler {
    Class<? extends Command> getCommandClass();
    Result execute(Command c);
}
