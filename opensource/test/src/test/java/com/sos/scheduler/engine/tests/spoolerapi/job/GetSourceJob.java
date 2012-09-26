package com.sos.scheduler.engine.tests.spoolerapi.job;

import sos.spooler.Job_impl;

public class GetSourceJob extends Job_impl {
    @Override public boolean spooler_process() {
        spooler.variables().set_value(spooler_task.job().name(),spooler_task.job().script_code());
        return false;
    }
}
