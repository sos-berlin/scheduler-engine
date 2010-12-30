package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.kernel.log.LogEvent;
import com.sos.scheduler.kernel.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public class ErrorLogEvent extends LogEvent {
    public ErrorLogEvent(String message) {
        super(message);
    }
}
