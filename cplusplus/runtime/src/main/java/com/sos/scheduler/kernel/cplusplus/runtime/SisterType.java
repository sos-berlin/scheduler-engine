package com.sos.scheduler.kernel.cplusplus.runtime;


public interface SisterType<SISTER extends Sister, PROXY extends CppProxyWithSister<SISTER>>
{
    /** Liefert die Java-Schwester eines C++-Objekts. */
    SISTER sister(PROXY cppProxy, Sister context);
}
