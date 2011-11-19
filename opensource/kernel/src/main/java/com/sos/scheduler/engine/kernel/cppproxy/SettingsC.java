package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;

@CppClass(clas="sos::scheduler::Settings", directory="scheduler", include="spooler.h")
public interface SettingsC extends CppProxy {
    //ModifiableSettings.Type sisterType = new ModifiableSettings.Type();

    void set_db_name(String o);
}
