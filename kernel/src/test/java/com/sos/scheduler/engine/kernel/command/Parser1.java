package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;


class Parser1 extends SingleXmlCommandParser {
    static Parser1 singleton = new Parser1();

    public Parser1() {
        super("command1");
    }

    @Override public final Command parse(Element e) {
        return new Command1(e.getAttribute("value1"));
    }
}
