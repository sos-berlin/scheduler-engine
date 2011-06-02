package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::Database", directory="scheduler", include="spooler.h")
public interface DatabaseC extends CppProxy {
    //Database.Type sisterType = new Database.Type();

    String db_name();       // Mit Kennwort
    String job_history_tablename();
}
