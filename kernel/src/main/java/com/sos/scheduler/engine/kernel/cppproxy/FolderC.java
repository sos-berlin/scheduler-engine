package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.folder.Folder;

@CppClass(clas = "sos::scheduler::Folder", directory = "scheduler", include = "spooler.h")
public interface FolderC extends CppProxyWithSister<Folder>, File_basedC<Folder> {
    Folder.Type sisterType = new Folder.Type();
}
