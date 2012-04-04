package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;

public class SchedulerLog {
    private final SpoolerC spoolerC;

    public SchedulerLog(SpoolerC spoolerC) {
        this.spoolerC = spoolerC;
    }

    public final void write(LogCategory logCategory, String text) {
        try {
            if (spoolerC.cppReferenceIsValid())
                spoolerC.write_to_scheduler_log(logCategory.asString(), text);
        }
        catch (Throwable x) {
            // Keine Log-Ausgabe, weil wir vielleicht schon von einem Logger aufgerufen worden sind.
        }
    }
}
