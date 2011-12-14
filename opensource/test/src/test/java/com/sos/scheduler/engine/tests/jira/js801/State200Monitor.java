package com.sos.scheduler.engine.tests.jira.js801;

public class State200Monitor extends sos.spooler.Monitor_impl {
    @Override public boolean spooler_process_after(boolean result) {
        spooler_task.order().set_state("200");
        return result;
    }
}
