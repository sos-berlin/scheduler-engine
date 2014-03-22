package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.data.job.TaskPersistentState;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.scheduler.HasInjector;

@CppClass(clas="sos::scheduler::Job", directory="scheduler", include="spooler.h")
public interface JobC extends CppProxyWithSister<Job>, File_basedC<Job> {
    SisterType<Job, JobC> sisterType = new SisterType<Job, JobC>() {
        public Job sister(JobC proxy, Sister context) {
            return new Job(proxy, ((HasInjector)context).injector());
        }
    };

    String description();
    String state_name();
    void set_state_cmd(String cmd);
    boolean is_permanently_stopped();
    long next_start_time_millis();
    void enqueue_taskPersistentState(TaskPersistentState o);
}
