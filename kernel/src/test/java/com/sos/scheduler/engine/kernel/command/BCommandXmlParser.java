package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;


class BCommandXmlParser extends SingleCommandXmlParser {
    static BCommandXmlParser singleton = new BCommandXmlParser();


    public BCommandXmlParser() {
        super("b");
    }

    
    @Override
    public final Command parse(Element e) {
        return new BCommand(e.getAttribute("value2"));
    }
}
