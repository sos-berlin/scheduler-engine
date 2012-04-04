package com.sos.scheduler.engine.data.folder;

import com.sos.scheduler.engine.data.event.AbstractEvent;

public class AbstractFileBasedEvent extends AbstractEvent implements FileBasedEvent {
    private final TypedPath typedPath;

    public AbstractFileBasedEvent(TypedPath o) {
        this.typedPath = o;
    }

    @Override public final TypedPath getTypedPath() {
        return typedPath;
    }
}

