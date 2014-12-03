package com.sos.scheduler.engine.tests.jira.js1103;

import sos.spooler.Job_impl;
import sos.spooler.Variable_set;

public class OrderJob1 extends Job_impl {

    @Override public final boolean spooler_process() {

        Variable_set params = spooler_task.order().params();

        if ( params.value("suspend").equals("yes") )
            return false; // on_error -> suspend order to keep it in job_chain to reach max_orders=1
        else
            return true;
    }
}
