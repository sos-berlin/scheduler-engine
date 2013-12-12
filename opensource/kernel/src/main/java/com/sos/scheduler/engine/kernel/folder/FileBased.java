package com.sos.scheduler.engine.kernel.folder;

import java.util.UUID;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.data.folder.AbsolutePath;
import com.sos.scheduler.engine.data.folder.FileBasedType;
import com.sos.scheduler.engine.data.folder.TypedPath;
import com.sos.scheduler.engine.eventbus.EventSource;

@ForCpp
public abstract class FileBased implements Sister, EventSource {
    private final UUID uuid = UUID.randomUUID();

    /** Jedes Exemplar hat seine eigene UUID. */
    public final UUID getUuid() {
        return uuid;
    }

    public final TypedPath getTypedPath() {
        return getFileBasedType().typedPath(getPath().string());
    }

    public abstract FileBasedType getFileBasedType();

    public abstract AbsolutePath getPath();
}
