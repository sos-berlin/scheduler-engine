package com.sos.scheduler.engine.kernel.plugin;


public class ErrorPlugInMock implements PlugIn {
    @Override public void activate() {
        throw new RuntimeException("activate");
    }

    @Override public void close() {
        throw new RuntimeException("close");
    }

    @Override public String getXmlState() {
        throw new RuntimeException("getXmlState");
    }
}
