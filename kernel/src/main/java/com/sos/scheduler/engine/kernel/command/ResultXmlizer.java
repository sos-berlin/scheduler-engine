package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;


public interface ResultXmlizer {
    Class<? extends Result> getResultClass();
    Element toElement(Result r);
}
