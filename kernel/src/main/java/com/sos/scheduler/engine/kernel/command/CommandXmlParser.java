package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;


public interface CommandXmlParser {
    Command parse(Element e);
}
