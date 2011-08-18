package com.sos.scheduler.engine.kernel.plugin;


/** Erleichtert die Implementierung eines Plugin. */
public abstract class AbstractPlugin implements PlugIn {
    @Override public void activate() {
    }


    @Override public void close() {
    }


    @Override public String getXmlState() {
        return "";
    }
}
