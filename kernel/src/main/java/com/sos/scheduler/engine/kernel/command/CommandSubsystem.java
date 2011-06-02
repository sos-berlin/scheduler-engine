package com.sos.scheduler.engine.kernel.command;

import com.sos.scheduler.engine.kernel.Subsystem;


public class CommandSubsystem implements Subsystem {
    private final CommandDispatcher commandDispatcher;


    public CommandSubsystem(Iterable<CommandSuite> commandSuites) {
        commandDispatcher = new CommandDispatcher(commandSuites);
    }

    
    public String executeXml(String xml) {
        return commandDispatcher.executeXml(xml);
    }
}
