package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.kernel.SchedulerObject;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;


public class FolderEvent extends ObjectEvent {
	private final FileBased fileBased;
	
    public FolderEvent(FileBased o) {
        fileBased = o;
    }

    public final FileBased getFolder() {
        return fileBased;
    }

	protected SchedulerObject getObject() {
		return fileBased;
	}
}
