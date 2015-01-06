package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.lock.Lock;

@CppClass(clas="sos::scheduler::lock::Lock", directory="scheduler", include="spooler.h")
public interface LockC extends CppProxyWithSister<Lock>, File_basedC<Lock> {
    Lock.Type sisterType = new Lock.Type();
}
