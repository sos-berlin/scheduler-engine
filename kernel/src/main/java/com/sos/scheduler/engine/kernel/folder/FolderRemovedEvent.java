package com.sos.scheduler.engine.kernel.folder;

public class FolderRemovedEvent extends FolderEvent {
	
    public FolderRemovedEvent(FileBased folder) {
        super(folder);
    }

}
