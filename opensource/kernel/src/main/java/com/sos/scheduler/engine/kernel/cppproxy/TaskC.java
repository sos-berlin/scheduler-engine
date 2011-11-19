package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.job.Task;

@CppClass(clas="sos::scheduler::Task", directory="scheduler", include="spooler.h")
public interface TaskC extends CppProxyWithSister<Task> {
    Task.Type sisterType = new Task.Type();
}
