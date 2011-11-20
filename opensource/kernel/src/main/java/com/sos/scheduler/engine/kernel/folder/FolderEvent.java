package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerObject;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;

public abstract class FolderEvent extends ObjectEvent {
	private final FileBased fileBased;
	
    protected FolderEvent(FileBased o) {
        fileBased = o;
    }

    public final FileBased getFolder() {
        return fileBased;
    }

	@Override protected final SchedulerObject getObject() {
		return fileBased;
	}
}
