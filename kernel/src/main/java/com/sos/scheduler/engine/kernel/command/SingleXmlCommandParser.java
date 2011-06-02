package com.sos.scheduler.engine.kernel.command;

public abstract class SingleXmlCommandParser implements XmlCommandParser {
    private final String commandName;


    public SingleXmlCommandParser(String commandName) {
        this.commandName = commandName;
    }

    
    public String getCommandName() {
        return commandName;
    }
}
