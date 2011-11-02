package com.sos.scheduler.engine.kernel.cppproxy;

import java.util.ArrayList;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.VariableSet;

@CppClass(clas="sos::scheduler::com_objects::Com_variable_set", directory="scheduler", include="spooler.h")
public interface Variable_setC extends CppProxyWithSister<VariableSet> {
    VariableSet.Type sisterType = new VariableSet.Type();

    int count();
    ArrayList<String> java_names();
    String get_string(String name);
    void set_var(String name, String value);
}
