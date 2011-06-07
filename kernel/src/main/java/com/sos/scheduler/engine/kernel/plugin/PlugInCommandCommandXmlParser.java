package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.Command;
import com.sos.scheduler.engine.kernel.command.PrefixCommandXmlParser;
import org.w3c.dom.Element;


public class PlugInCommandCommandXmlParser extends PrefixCommandXmlParser {
    public PlugInCommandCommandXmlParser() {
        super("plugin.");
    }


    @Override public Command parse(Element e) {
        return new PlugInCommandCommand(e.getAttribute("plugin_class"), e);
    }
}
