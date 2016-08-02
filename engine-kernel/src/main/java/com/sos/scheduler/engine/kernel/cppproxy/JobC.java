package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.data.job.TaskPersistentState;
import com.sos.scheduler.engine.kernel.job.Job;

@CppClass(clas="sos::scheduler::Job", directory="scheduler", include="spooler.h")
public interface JobC extends CppProxyWithSister<Job>, File_basedC<Job> {

    Job.Type sisterType = new Job.Type();

    String default_process_class_path();
    boolean is_in_period();
    int max_tasks();
    int running_tasks_count();
    String script_text();
    String title();
    String description();
    String state_name();
    String state_text();
    void set_state_cmd(String cmd);
    boolean is_permanently_stopped();
    long next_start_time_millis();
    long next_possible_start_millis();
    void enqueue_taskPersistentState(TaskPersistentState o);
    boolean waiting_for_process();
}
