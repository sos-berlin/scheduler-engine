package com.sos.scheduler.engine.tests.excluded.ss.runtime;

import sos.spooler.Job_impl;

public class JobSleep extends Job_impl {
    @Override public final boolean spooler_process() {
        spooler_log.info("Hej!");
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return true;
    }
}
