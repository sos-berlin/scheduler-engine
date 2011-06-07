package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


class BResultXmlizer extends GenericResultXmlizer<BResult> {
    static BResultXmlizer singleton = new BResultXmlizer();


    public BResultXmlizer() {
        super(BResult.class);
    }


    @Override public final Element doToElement(BResult r) {
        Element e = newDocument().createElement("bResult");
        e.setAttribute("value2", r.value);
        return e;
    }
}
