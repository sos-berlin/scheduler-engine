package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog;

import sos.spooler.Job_impl;

public class JobDelayTask extends Job_impl {
    @Override
    public boolean spooler_process() {
        spooler_task.set_delay_spooler_process(1);
        return true;
    }
}
