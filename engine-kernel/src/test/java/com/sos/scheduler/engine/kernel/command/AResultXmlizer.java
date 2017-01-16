package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;

import static com.sos.scheduler.engine.common.xml.CppXmlUtils.newDocument;

class AResultXmlizer extends GenericResultXmlizer<AResult> {
    static final AResultXmlizer singleton = new AResultXmlizer();

    AResultXmlizer() {
        super(AResult.class);
    }

    @Override public final Element doToElement(AResult r) {
        Element e = newDocument().createElement("aResult");
        e.setAttribute("value1", r.value);
        return e;
    }
}
