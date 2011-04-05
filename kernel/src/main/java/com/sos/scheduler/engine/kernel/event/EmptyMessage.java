package com.sos.scheduler.engine.kernel.event;


public final class EmptyMessage extends SimpleMessage {
    private EmptyMessage() {
        super("(empty message)");
    }

    public static final EmptyMessage singleton = new EmptyMessage();
}
