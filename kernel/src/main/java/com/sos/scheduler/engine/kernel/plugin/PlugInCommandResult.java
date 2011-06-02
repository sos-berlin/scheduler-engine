package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.Result;
import org.w3c.dom.Element;


public class PlugInCommandResult implements Result {
    private final Element element;


    public PlugInCommandResult(Element e) {
        element = e;
    }

    
    public Element getElement() {
        return element;
    }
}
