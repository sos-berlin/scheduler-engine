package com.sos.scheduler.engine.kernel.command;

public abstract class SingleCommandXmlParser implements CommandXmlParser {
    private final String commandName;


    public SingleCommandXmlParser(String commandName) {
        this.commandName = commandName;
    }

    
    public String getCommandName() {
        return commandName;
    }
}
