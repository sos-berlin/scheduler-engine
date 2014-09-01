package com.sos.scheduler.engine.tests.jobchain.max_order;

import sos.spooler.Job_impl;

public final class DelayingJob extends Job_impl {
    @Override
    public boolean spooler_process() throws InterruptedException {
        Thread.sleep(1000);
        return true;
    }
}
