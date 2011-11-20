package com.sos.scheduler.engine.main;

/** Die Zustände werden nacheinander, ohne Wiederholung, durchlaufen. */
public enum SchedulerState {
    /** Scheduler startet und kann noch nicht benutzt werden. */
    starting,

    /** Scheduler ist gestartet, aber noch nicht aktiv. Scheduler kann teilweise benutzt werden. */
    started,

    /** Scheduler läuft. */
    active,

    /** Das Java_subsystem des C++-Scheduler ist geschlossen. */
    closed,

    /** C++-Scheduler hat sich beendet. Als nächstes beendet sich der Thread. */
    terminated
}
