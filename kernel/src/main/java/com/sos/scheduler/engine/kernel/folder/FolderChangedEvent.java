package com.sos.scheduler.engine.kernel.folder;

public class FolderChangedEvent extends FolderEvent {
    public FolderChangedEvent(FileBased folder) {
        super(folder);
    }
}
