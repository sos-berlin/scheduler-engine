package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

/** Eine Meldung im Hauptprotokoll des Gewichts "error". */
@ForCpp
public class ErrorLogEvent extends LogEvent {
    @ForCpp public ErrorLogEvent(String message) {
        super(message);
    }
}
