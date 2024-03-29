package com.sos.scheduler.engine.data.scheduler;

import java.util.NoSuchElementException;

/**
 * @author Joacim Zschimmer
 */
public enum SchedulerState {
    none("none"),
    stopped("stopped"),
    loading("loading"),
    starting("starting"),
    waiting_for_activation("waiting_for_activation"),
    waiting_for_activation_paused("waiting_for_activation_paused"),
    running("running"),
    paused("paused"),
    stopping("stopping"),
    stopping_let_run("stopping_let_run");

    private final String cppName;

    SchedulerState(String cppName) {
        this.cppName = cppName;
    }

    public String cppName() {
        return cppName;
    }

    @Override public String toString() {
        return cppName;
    }

    public static SchedulerState ofCppName(String name) {
        for (SchedulerState v : values()) if (v.cppName.equals(name)) return v;
        throw new NoSuchElementException("Unknown SchedulerState '" + name + "'");
    }
}
