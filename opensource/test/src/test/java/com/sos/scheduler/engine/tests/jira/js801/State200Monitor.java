package com.sos.scheduler.engine.tests.jira.js801;

import sos.spooler.Monitor_impl;

public class State200Monitor extends Monitor_impl {
    @Override public boolean spooler_process_after(boolean result) {
        spooler_task.order().set_state("200");
        return result;
    }
}
