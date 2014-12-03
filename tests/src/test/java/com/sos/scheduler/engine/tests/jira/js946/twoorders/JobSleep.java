package com.sos.scheduler.engine.tests.jira.js946.twoorders;

import sos.spooler.Job_impl;

public class JobSleep extends Job_impl {
    @Override public final boolean spooler_process() throws InterruptedException {
        spooler_log.info("Hej!");
        Thread.sleep(3000);
        return true;
    }
}
