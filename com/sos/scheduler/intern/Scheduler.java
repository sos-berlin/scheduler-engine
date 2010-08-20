package com.sos.scheduler.intern;
import com.sos.scheduler.intern.cppproxy.*;

class Scheduler
{
    private final SpoolerC spoolerC;
    
    
    public Scheduler(SpoolerC spoolerC) {
        this.spoolerC = spoolerC;
    }
    
    
    public void test(String message) {
        String httpUrl = spoolerC.http_url();
        System.err.println("url=" + httpUrl);
        //spoolerC.log().info("MELDUNG AUS JAVA: " + message);
    }
};
