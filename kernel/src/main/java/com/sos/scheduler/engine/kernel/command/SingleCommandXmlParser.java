package com.sos.scheduler.engine.kernel.command;

/** Parser für ein bestimmtes, benanntes Kommando. */
public abstract class SingleCommandXmlParser implements CommandXmlParser {
    private final String commandName;

    protected SingleCommandXmlParser(String commandName) {
        this.commandName = commandName;
    }

    public final String getCommandName() {
        return commandName;
    }
}
