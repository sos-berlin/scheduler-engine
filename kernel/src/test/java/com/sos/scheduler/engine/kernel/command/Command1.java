package com.sos.scheduler.engine.kernel.command;


class Command1 implements Command {
    final String value;


    public Command1(String v) {
        value = v;
    }


    @Override public final String getName() {
        return "command1";
    }
}
