package com.sos.scheduler.engine.kernel.command;


class BCommand implements Command {
    final String value;


    public BCommand(String v) {
        value = v;
    }


    @Override public final String getName() {
        return "b";
    }
}
