package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.data.job.TaskPersistent;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.scheduler.HasInjector;

@CppClass(clas="sos::scheduler::Job", directory="scheduler", include="spooler.h")
public interface JobC extends CppProxyWithSister<Job> {
    SisterType<Job, JobC> sisterType = new SisterType<Job, JobC>() {
        public Job sister(JobC proxy, Sister context) {
            return new Job(proxy, ((HasInjector)context).getInjector());
        }
    };

    String file_based_state_name();
    void set_force_file_reread();
    boolean is_file_based_reread();
    String name();
    String path();
    String source_xml();
    String description();
    Prefix_logC log();
    String state_name();
    void set_state_cmd(String cmd);
    boolean is_permanently_stopped();
    long next_start_time_millis();
    void enqueue_task(TaskPersistent o);
}
