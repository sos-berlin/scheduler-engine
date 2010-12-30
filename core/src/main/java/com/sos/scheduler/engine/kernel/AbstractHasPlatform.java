package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.log.PrefixLog;


public abstract class AbstractHasPlatform implements HasPlatform {
    protected final Platform platform;
    private PrefixLog log;


    protected AbstractHasPlatform(Platform platform) {
        this.platform = platform;
    }

//    protected AbstractSchedulerObject(SpoolerC spoolerC) {
//        this.scheduler = Scheduler.of(spoolerC);
//    }


    @Override public Platform getPlatform() { return platform; }


    @Override public PrefixLog log() {
        if (log == null)  log = platform.log();
        return log;
    }
}
