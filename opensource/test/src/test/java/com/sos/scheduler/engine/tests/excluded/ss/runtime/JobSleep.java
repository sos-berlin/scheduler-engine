package com.sos.scheduler.engine.tests.excluded.ss.runtime;

import sos.spooler.Job_impl;

public class JobSleep extends Job_impl {
    @Override public final boolean spooler_process() throws InterruptedException {
        spooler_log.info("Hej!");
        Thread.sleep(2000);
        return true;
    }
}
