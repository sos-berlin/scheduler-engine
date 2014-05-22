package com.sos.scheduler.engine.kernel.plugin;


public class ErrorMockPlugin extends AbstractPlugin {
    @Override public void onPrepare() {
        throw new RuntimeException("prepare");
    }

    @Override public void onActivate() {
        throw new RuntimeException("activate");
    }

    @Override public void close() {
        throw new RuntimeException("close");
    }

    @Override public String xmlState() {
        throw new RuntimeException("xmlState");
    }
}
