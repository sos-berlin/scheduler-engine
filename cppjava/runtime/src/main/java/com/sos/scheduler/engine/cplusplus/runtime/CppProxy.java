package com.sos.scheduler.engine.cplusplus.runtime;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.JavaOnlyInterface;


@JavaOnlyInterface
public interface CppProxy {
    ThreadLock threadLock = new ThreadLock();
    
    boolean cppReferenceIsValid();
}
