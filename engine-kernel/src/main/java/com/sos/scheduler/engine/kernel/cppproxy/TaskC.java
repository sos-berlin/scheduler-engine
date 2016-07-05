package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.job.Task;

@CppClass(clas="sos::scheduler::Task", directory="scheduler", include="spooler.h")
public interface TaskC extends CppProxyWithSister<Task> {

    SisterType<Task, TaskC> sisterType = (proxy, context) -> new Task(proxy);

    int id();
    JobC job();
    String job_path();
    String state_name();
    String process_class_path();
    String remote_scheduler_address();
    OrderC order();
    Variable_setC params();
    String log_string();
    String stdout_path();
    String stderr_path();
    Prefix_logC log();
}
