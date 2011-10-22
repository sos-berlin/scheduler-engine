package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

public class FolderAttachedEvent extends FolderEvent {
    @ForCpp public FolderAttachedEvent(FileBased folder) {
        super(folder);
    }
}
