package com.sos.scheduler.engine.kernel.plugin;


public class ErrorMockPlugin implements Plugin {
    @Override public void activate() {
        throw new RuntimeException("activate");
    }

    @Override public void close() {
        throw new RuntimeException("close");
    }

    @Override public String xmlState() {
        throw new RuntimeException("getXmlState");
    }
}
