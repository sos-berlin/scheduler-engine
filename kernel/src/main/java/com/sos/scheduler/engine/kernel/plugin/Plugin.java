package com.sos.scheduler.engine.kernel.plugin;


public interface Plugin {
    void activate();
    void close();
    String getXmlState();   // Für <show_state>

    // Außerdem eine statische Methode, die eine PluginFactory liefert und die vom Scheduler aufgerufen wird:
    // public static PluginFactory newFactory();
}
