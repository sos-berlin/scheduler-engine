package com.sos.scheduler.engine.cplusplus.runtime;


public interface SisterType<SISTER extends Sister, PROXY extends CppProxyWithSister<SISTER>>
{
    /** Liefert die Java-Schwester eines C++-Objekts. */
    SISTER sister(PROXY cppProxy, Sister context);
}
