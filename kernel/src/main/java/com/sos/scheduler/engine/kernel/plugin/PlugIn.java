package com.sos.scheduler.engine.kernel.plugin;


public interface PlugIn {
//    void initialize();
//    void load();
    void activate();
    void close();
    String getXmlState();

    // Außerdem eine statische Methode, die eine PlugInFactory liefert und die vom Scheduler aufgerufen wird:
    // public static PlugInFactory newFactory();
}
