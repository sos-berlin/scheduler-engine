package com.sos.scheduler.engine.kernel.plugin;

public interface Plugin {
    String configurationXMLName = "com.sos.scheduler.engine.kernel.plugin.Plugin.configurationXML";

    void activate();
    void close();
    String xmlState();   // Für <show_state>
}
