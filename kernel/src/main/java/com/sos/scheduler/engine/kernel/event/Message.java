package com.sos.scheduler.engine.kernel.event;


public class Message {
    public static final Message empty = new Message("(empty message)");

    private final String code;


    public Message(String code) {
        this.code = code;
    }

    
    public String getCode() { return code; }
    
    @Override public String toString() { return code; }
}
