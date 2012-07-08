package com.sos.scheduler.engine.kernel.job;

public enum JobStateCommand {
    stop("stop"),
    unstop("unstop"),
    start("start"),
    wake("wake"),
    endTasks("end"),
    suspendTasks("suspend"),
    continueTasks("continue"),
  //reread("reread"),
    remove("remove"),
    enable("enable"),
    disable("disable");

    private final String cppValue;

    JobStateCommand(String cppValue) {
        this.cppValue = cppValue;
    }

    public final String cppValue() {
        return cppValue;
    }
}
