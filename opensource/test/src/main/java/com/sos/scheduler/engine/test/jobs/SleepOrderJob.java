package com.sos.scheduler.engine.test.jobs;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import sos.spooler.Job_impl;

public class SleepOrderJob extends Job_impl {

    private static final String blank = "";
    private static final String parameterName = "RUNTIME_IN_SECONDS";
    private static final long defaultRuntime = 5;
    
    @Override public boolean spooler_process() {
        String param = spooler_task.order().params().value(parameterName);
        long runtime = ((param.equals(blank)) ?  defaultRuntime : Integer.parseInt(param)) * 1000;
        try {
            Thread.sleep( runtime );
        } catch (InterruptedException e) {
            throw new SchedulerException("error processing Thread.sleep(" + runtime + ")",e);
        }
        return true;
    }
}
