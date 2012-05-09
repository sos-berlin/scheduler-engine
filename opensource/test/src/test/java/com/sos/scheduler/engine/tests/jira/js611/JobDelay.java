package com.sos.scheduler.engine.tests.jira.js611;

import sos.spooler.Job_impl;

public class JobDelay extends Job_impl {
    @Override
    public boolean spooler_process() {
        spooler_log.info("Hej!");
        spooler_task.set_delay_spooler_process(1);
        return true;
    }
}
