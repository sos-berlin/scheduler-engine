package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;


public class SchedulerLog {
    private final SpoolerC spoolerC;


    public SchedulerLog(SpoolerC spoolerC) {
        this.spoolerC = spoolerC;
    }

    
    public void write(LogCategory logCategory, String text) {
        spoolerC.write_to_scheduler_log(logCategory.string(), text);
    }
}
