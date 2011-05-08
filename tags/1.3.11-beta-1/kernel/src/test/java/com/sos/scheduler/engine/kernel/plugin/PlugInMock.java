package com.sos.scheduler.engine.kernel.plugin;


public class PlugInMock implements PlugIn {
    @Override public void activate() {
    }

    @Override public void close() {
    }

    @Override public String getXmlState() {
        return "<plugInMock/>";
    }
}
