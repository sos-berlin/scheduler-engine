package com.sos.scheduler.intern;

import com.sos.scheduler.intern.cppproxy.*;


class Scheduler
{
    private final SpoolerC spoolerC;
    
    
    public Scheduler(SpoolerC spoolerC) {
        this.spoolerC = spoolerC;
    }
    
    
    public void test(String message) {
        spoolerC.log().info("MELDUNG AUS JAVA: " + message + " -- http_url=" + spoolerC.http_url());
        
        Job_subsystemC jobSubsystem = spoolerC.job_subsystem();
        for(int i=0; i < 1000000; i++)  spoolerC.cmd_terminate(false,10);
        JobC job = jobSubsystem.job_by_string("/scheduler_file_order_sink");
        spoolerC.log().info("job.name=" + job.name());
    }
}
