package com.sos.scheduler.engine.kernel.command;


public abstract class PrefixXmlCommandParser implements XmlCommandParser {
    public static final String prefixTerminator = ".";

    private final String prefix;


    public PrefixXmlCommandParser(String commandPrefix) {
        this.prefix = commandPrefix;
        assert prefix.endsWith(prefixTerminator);
    }


    public String getCommandPrefix() {
        return prefix;
    }
}
