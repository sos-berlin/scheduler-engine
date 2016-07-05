package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.processclass.ProcessClass;

@CppClass(clas="sos::scheduler::Process_class", directory="scheduler", include="spooler.h")
public interface Process_classC extends CppProxyWithSister<ProcessClass>, File_basedC<ProcessClass> {

    ProcessClass.Type sisterType = new ProcessClass.Type();

    int max_processes();
    int used_process_count();
}
