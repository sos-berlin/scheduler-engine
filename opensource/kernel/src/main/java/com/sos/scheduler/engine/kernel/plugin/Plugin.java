package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;

public interface Plugin extends EventHandlerAnnotated {
    void activate();
    void close();
    String getXmlState();   // Für <show_state>

    // Außerdem eine statische Methode, die eine PluginFactory liefert und die vom Scheduler aufgerufen wird:
    // public static PluginFactory newFactory();
}
