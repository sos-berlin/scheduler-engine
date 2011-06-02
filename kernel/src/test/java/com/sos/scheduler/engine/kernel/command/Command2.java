package com.sos.scheduler.engine.kernel.command;


class Command2 implements Command {
    final String value;


    public Command2(String v) {
        value = v;
    }


    @Override public final String getName() {
        return "command2";
    }
}
