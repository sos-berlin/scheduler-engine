package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.Command;
import org.w3c.dom.Element;


public class PlugInCommandCommand implements Command {
    private final String pluginClassName;
    private final Element element;


    public PlugInCommandCommand(String pluginClassName, Element e) {
        this.pluginClassName = pluginClassName;
        this.element = e;
    }


    @Override public String getName() {
        return "plugin.command";
    }


    public final String getPluginClassName() {
        return pluginClassName;
    }

    
    public final Element getElement() {
        return element;
    }
}
