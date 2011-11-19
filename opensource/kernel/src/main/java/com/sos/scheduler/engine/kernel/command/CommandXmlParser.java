package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;


public interface CommandXmlParser extends CommandHandler {
    Command parse(Element e);
}
