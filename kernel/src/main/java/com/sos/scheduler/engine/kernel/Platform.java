package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.kernel.cplusplus.runtime.Sister;
import com.sos.scheduler.kernel.cplusplus.runtime.annotation.ForCpp;


/** Minimale Voraussetzungen für Scheduler-bewusste Objekte.
 * Mit möglichst wenig Abhängigkeiten.
 */
@ForCpp
public class Platform {
    private final PrefixLog log;


    public Platform(PrefixLog log) {
        this.log = log;
    }

    
    public PrefixLog log() { return log; }


    public static Platform of(Sister scheduler) {
        // In C++ haben wir Platform noch nicht. Also nehmen wir Scheduler.
        assert scheduler instanceof Scheduler;
        
        return ((Scheduler)scheduler).getPlatform();
    }
}
