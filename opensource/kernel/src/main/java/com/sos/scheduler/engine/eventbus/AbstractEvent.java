package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

/* Für den C++-Code muss ein Event von dieser Klasse erben, denn der C++/Java-Generator berücksichtigt Interfaces nicht. */
@ForCpp
public abstract class AbstractEvent implements Event {
    @Override public String toString() {
        return getClass().getSimpleName();
    }
}
