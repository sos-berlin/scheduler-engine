package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

public class FolderChangedEvent extends FolderEvent {
    @ForCpp public FolderChangedEvent(FileBased folder) {
        super(folder);
    }
}
