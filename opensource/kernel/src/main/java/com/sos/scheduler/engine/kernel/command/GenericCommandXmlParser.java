package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;


public abstract class GenericCommandXmlParser<C extends Command> extends SingleCommandXmlParser {
    public GenericCommandXmlParser(String commandName) {
        super(commandName);
    }

    
    @Override public final Command parse(Element e) {
        return doParse(e);
    }


    protected abstract Command doParse(Element e);
}
