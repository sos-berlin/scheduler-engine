package com.sos.scheduler.engine.tests.jira.js817;

import sos.spooler.Monitor_impl;

public class MonitorChangeState extends Monitor_impl {
    @Override
    public boolean spooler_process_after(boolean result) {
        spooler_task.order().set_state("200");
        return result;
    }
}
