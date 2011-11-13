package com.sos.scheduler.engine.kernel.event;


public interface EventSubscriber {
    /** Wird vom Scheduler bei einem Ereignis aufgerufen.
     * Weil wir hier mitten in der Scheduler-Ausführung sind,
     * sollte während dieses Aufrufs die C++-API vorsichtig benutzt werden.
     * Scheduler-API-Aufrufe können wieder Events auslösen, was zur Rekursion führt.
     * Nicht alle Aufrufe sind an jeder Stelle möglich.
     */
    void onEvent(Event e) throws Exception;
}
