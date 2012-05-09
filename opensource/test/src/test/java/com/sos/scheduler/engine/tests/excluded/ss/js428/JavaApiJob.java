package com.sos.scheduler.engine.tests.excluded.ss.js428;

import sos.spooler.Job_impl;

public class JavaApiJob extends Job_impl {
    @Override
    public boolean spooler_process() {
        String p1 = spooler_task.order().params().value("param1");
        String p2 = spooler_task.order().params().value("param2");
        spooler_log.warn("param1=" + p1 );
        spooler_log.warn("param2=" + p2 );
        spooler.variables().set_var(spooler_job.name() + "_param1", p1);
        spooler.variables().set_var(spooler_job.name() + "_param2", p2);
        return true;
    }
}
