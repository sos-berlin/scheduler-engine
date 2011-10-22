package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

public class FolderRemovedEvent extends FolderEvent {
    @ForCpp public FolderRemovedEvent(FileBased folder) {
        super(folder);
    }
}
