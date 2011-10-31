package com.sos.scheduler.engine.kernel.folder.event;

import com.sos.scheduler.engine.kernel.event.ObjectEvent;
import com.sos.scheduler.engine.kernel.folder.FileBased;

@Deprecated     // Das ist zu allgemein. Die Ereignispunkte sind nicht ganz klar, vor allem für rekursive Aufrufe im Event-Handler
public class FileBasedStateChangedEvent extends ObjectEvent {
    private final FileBased fileBased;

    public FileBasedStateChangedEvent(FileBased fileBased) {
        this.fileBased = fileBased;
    }

    @Override public FileBased getObject() {
        return fileBased;
    }
}
