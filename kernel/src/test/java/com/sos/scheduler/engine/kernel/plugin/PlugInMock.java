package com.sos.scheduler.engine.kernel.plugin;


public class PluginMock implements Plugin {
    @Override public void activate() {
    }

    @Override public void close() {
    }

    @Override public String getXmlState() {
        return "<plugInMock/>";
    }
}
