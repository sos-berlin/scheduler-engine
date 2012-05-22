package com.sos.scheduler.engine.tests.jobchain.max_order;

import sos.spooler.Job_impl;

public class DelayingJob extends Job_impl {
    @Override
    public boolean spooler_process() {
        spooler_task.set_delay_spooler_process(1);
        return true;
    }
}
