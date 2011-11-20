package com.sos.scheduler.engine.kernel.command;

import com.sos.scheduler.engine.kernel.scheduler.Subsystem;


public class CommandSubsystem implements Subsystem {
    private final CommandDispatcher commandDispatcher;


    public CommandSubsystem(Iterable<CommandHandler> commandHandlers) {
        commandDispatcher = new CommandDispatcher(commandHandlers);
    }

    
    public final String executeXml(String xml) {
        return commandDispatcher.executeXml(xml);
    }
}
