package com.sos.scheduler.engine.kernel.folder.events;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.AbstractEvent;

@ForCpp
public class FileBasedActivatedEvent extends AbstractEvent {
    @ForCpp public FileBasedActivatedEvent() {}
}
