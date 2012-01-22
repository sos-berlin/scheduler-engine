package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;

import java.util.UUID;

@ForCpp
public abstract class FileBased extends AbstractHasPlatform implements Sister, EventSource {
    private final UUID uuid = UUID.randomUUID();

    protected FileBased(Platform p) {
        super(p);
    }

    /** Jedes Exemplar hat seine eigene UUID. */
    public final UUID getUuid() {
        return uuid;
    }
}
