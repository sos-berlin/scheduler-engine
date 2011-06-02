package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.Command;
import com.sos.scheduler.engine.kernel.command.PrefixXmlCommandParser;
import org.w3c.dom.Element;


public class PlugInCommandParser extends PrefixXmlCommandParser {
    public PlugInCommandParser() {
        super("plugin.");
    }


    @Override public Command parse(Element e) {
        return new PlugInCommandCommand(e.getAttribute("plugin_class"), e);
    }
}
