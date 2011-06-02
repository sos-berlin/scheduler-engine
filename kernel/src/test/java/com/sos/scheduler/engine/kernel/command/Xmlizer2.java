package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


class Xmlizer2 extends GenericResultXmlizer<Result2> {
    static Xmlizer2 singleton = new Xmlizer2();


    public Xmlizer2() {
        super(Result2.class);
    }


    @Override public final Element doToElement(Result2 r) {
        Element e = newDocument().createElement("result2");
        e.setAttribute("value2", r.value);
        return e;
    }
}
