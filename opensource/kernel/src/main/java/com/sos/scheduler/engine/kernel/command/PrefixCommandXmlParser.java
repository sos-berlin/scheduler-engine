package com.sos.scheduler.engine.kernel.command;


public abstract class PrefixCommandXmlParser implements CommandXmlParser {
    public static final String prefixTerminator = ".";

    private final String prefix;


    public PrefixCommandXmlParser(String commandPrefix) {
        this.prefix = commandPrefix;
        assert prefix.endsWith(prefixTerminator);
    }


    public final String getCommandPrefix() {
        return prefix;
    }
}
