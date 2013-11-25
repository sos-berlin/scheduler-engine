package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.async.CppCall;

@CppClass(clas="sos::scheduler::Timed_call", directory="scheduler", include="spooler.h")
public interface Timed_callC extends CppProxyWithSister<CppCall> {

    SisterType<CppCall, Timed_callC> sisterType = new SisterType<CppCall, Timed_callC>() {
        public CppCall sister(Timed_callC proxy, Sister context) {
            return new CppCall(proxy);
        }
    };

    long at_millis();
    Object value();
    void set_value(Object o);
    void call();
    String obj_name();
}
