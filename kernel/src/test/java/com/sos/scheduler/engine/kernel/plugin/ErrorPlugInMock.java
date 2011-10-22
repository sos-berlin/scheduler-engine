package com.sos.scheduler.engine.kernel.plugin;


public class ErrorPluginMock implements Plugin {
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
