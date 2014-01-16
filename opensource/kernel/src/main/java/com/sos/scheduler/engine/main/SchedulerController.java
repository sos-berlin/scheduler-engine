package com.sos.scheduler.engine.main;

import com.sos.scheduler.engine.common.time.Time;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;

/** Steuerung für den C++-Scheduler in einem eigenen nebenläufigen Thread. */
public interface SchedulerController {
    /** Startet den Scheduler ohne zu warten. */
    void startScheduler(Iterable<String> arguments);

    /** Veranlasst die Beendigung des Schedulers, wartet aufs Ende und schließt alles. */
    void close();

    /** Veranlasst die Beendigung des Schedulers. */
    void terminateScheduler();

    /** Veranlasst die Beendigung des Schedulers nach einem Fehler.
     * Kann aus einem anderen Thread aufgerufen werden und lässt die Warte-Methoden die Exception werfen. */
    void terminateAfterException(Throwable x);

    boolean tryWaitForTermination(Time timeout);

    int exitCode();

    SchedulerEventBus getEventBus();
}
