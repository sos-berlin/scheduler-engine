package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.kernel.event.ObjectEvent;


public class FolderEvent extends ObjectEvent<FileBased> {
    public FolderEvent(FileBased o) {
        super(o);
    }

    public final FileBased getFolder() {
        return getObject();
    }
}
