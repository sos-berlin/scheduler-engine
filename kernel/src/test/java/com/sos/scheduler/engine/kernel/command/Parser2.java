package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;


class Parser2 extends SingleXmlCommandParser {
    static Parser2 singleton = new Parser2();


    public Parser2() {
        super("command2");
    }

    
    @Override
    public final Command parse(Element e) {
        return new Command2(e.getAttribute("value2"));
    }
}
