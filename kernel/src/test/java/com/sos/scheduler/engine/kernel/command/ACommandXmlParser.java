package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;

class ACommandXmlParser extends SingleCommandXmlParser {
    static final ACommandXmlParser singleton = new ACommandXmlParser();

    ACommandXmlParser() {
        super("a");
    }

    @Override public final Command parse(Element e) {
        return new ACommand(e.getAttribute("value1"));
    }
}
