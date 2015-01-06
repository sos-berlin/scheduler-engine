package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.folder.Folder;

@CppClass(clas="sos::scheduler::Folder_subsystem", directory="scheduler", include="spooler.h")
public interface Folder_subsystemC extends SubsystemC<Folder, FolderC>, CppProxy {
    boolean update_folders_now();
    String[] java_names(String absolute_path, String typeName);
}
