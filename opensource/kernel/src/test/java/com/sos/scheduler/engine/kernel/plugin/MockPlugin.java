package com.sos.scheduler.engine.kernel.plugin;

public class MockPlugin implements Plugin {
    @Override public void activate() {
    }

    @Override public void close() {
    }

    @Override public String xmlState() {
        return "<mockPlugin/>";
    }
}
