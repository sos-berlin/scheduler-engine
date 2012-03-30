package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

/** Eine Meldung im Hauptprotokoll des Gewichts "error". */
@ForCpp
public class InfoLogEvent extends LogEvent {
    @ForCpp public InfoLogEvent(String message) {
        super(message);
    }
}
