package com.sos.scheduler.engine.kernel.event;


public class SimpleMessage implements Message {

    private final String code;


    public SimpleMessage(String code) {
        this.code = code;
    }

    
    @Override public String getCode() { return code; }
    
    @Override public String toString() { return code; }
}
