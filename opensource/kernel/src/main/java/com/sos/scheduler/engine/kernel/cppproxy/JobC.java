package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.data.job.TaskPersistentState;
import com.sos.scheduler.engine.kernel.job.Job;

@CppClass(clas="sos::scheduler::Job", directory="scheduler", include="spooler.h")
public interface JobC extends CppProxyWithSister<Job>, File_basedC<Job> {
    Job.Type sisterType = new Job.Type();

    String script_text();
    String description();
    String state_name();
    void set_state_cmd(String cmd);
    boolean is_permanently_stopped();
    long next_start_time_millis();
    void enqueue_taskPersistentState(TaskPersistentState o);
    boolean waiting_for_process();
}
