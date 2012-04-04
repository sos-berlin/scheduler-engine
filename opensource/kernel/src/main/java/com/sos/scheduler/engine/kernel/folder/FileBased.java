package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.EventSource;

import java.util.UUID;

@ForCpp
public abstract class FileBased implements Sister, EventSource {
    private final UUID uuid = UUID.randomUUID();

    /** Jedes Exemplar hat seine eigene UUID. */
    public final UUID getUuid() {
        return uuid;
    }
}
