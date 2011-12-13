package com.sos.scheduler.engine.test.jobs;

import sos.spooler.Job_impl;

public class SingleStepJob extends Job_impl {
    @Override public boolean spooler_process() {
        return false;
    }
}
