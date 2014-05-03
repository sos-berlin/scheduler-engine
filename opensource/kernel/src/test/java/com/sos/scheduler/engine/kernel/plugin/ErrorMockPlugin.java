package com.sos.scheduler.engine.kernel.plugin;


public class ErrorMockPlugin extends AbstractPlugin {
    @Override public void prepare() {
        throw new RuntimeException("prepare");
    }

    @Override public void activate() {
        throw new RuntimeException("activate");
    }

    @Override public void close() {
        throw new RuntimeException("close");
    }

    @Override public String xmlState() {
        throw new RuntimeException("xmlState");
    }
}
