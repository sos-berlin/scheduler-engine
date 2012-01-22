package com.sos.scheduler.engine.kernel.scheduler;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;

import java.io.File;

import static com.google.common.base.Strings.isNullOrEmpty;

public final class Configuration {
    private final Scheduler scheduler;
    private final SpoolerC spoolerC;

    public Configuration(Scheduler scheduler, SpoolerC spoolerC) {
        this.scheduler = scheduler;
        this.spoolerC = spoolerC;
    }

    public File getLogDirectory() {
        String result = spoolerC.log_directory();
        if (isNullOrEmpty(result) || result.equals("*stderr")) throw new SchedulerException("Scheduler runs without a log directory");
        return new File(result);
    }
}
