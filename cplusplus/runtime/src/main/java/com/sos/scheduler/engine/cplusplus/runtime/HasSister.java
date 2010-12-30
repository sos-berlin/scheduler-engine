package com.sos.scheduler.engine.cplusplus.runtime;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.JavaOnlyInterface;


@JavaOnlyInterface
public interface HasSister<SISTER extends Sister>
{
    /** Zur freien Verfügung: Ein Schwester-Objekt kann zugeordnet werden */
    void setSister(SISTER o);
    boolean hasSister();
    SISTER getSister();
}
