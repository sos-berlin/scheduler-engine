package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.job.Task;

@CppClass(clas="sos::scheduler::Task", directory="scheduler", include="spooler.h")
public interface TaskC extends CppProxyWithSister<Task> {
    SisterType<Task, TaskC> sisterType = new SisterType<Task, TaskC>() {
        public Task sister(TaskC proxy, Sister context) {
            return new Task(proxy);
        }
    };

    int id();
    JobC job();
    OrderC order();
    Variable_setC params();
    String stdout_path();
    String stderr_path();
}
