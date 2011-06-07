package com.sos.scheduler.engine.kernel.command;


class ACommand implements Command {
    final String value;


    public ACommand(String v) {
        value = v;
    }


    @Override public final String getName() {
        return "a";
    }
}
