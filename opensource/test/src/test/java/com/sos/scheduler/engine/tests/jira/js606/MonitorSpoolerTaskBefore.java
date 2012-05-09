package com.sos.scheduler.engine.tests.jira.js606;

import sos.spooler.Monitor_impl;

public class MonitorSpoolerTaskBefore extends Monitor_impl {
    @Override
    public boolean spooler_task_before() {
        spooler_log.info("spooler_task_before");
        return true;
    }
}
