package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


class Xmlizer1 extends GenericResultXmlizer<Result1> {
    static Xmlizer1 singleton = new Xmlizer1();


    public Xmlizer1() {
        super(Result1.class);
    }


    @Override public final Element doToElement(Result1 r) {
        Element e = newDocument().createElement("result1");
        e.setAttribute("value1", r.value);
        return e;
    }
}
