package com.sos.scheduler.kernel.cplusplus.generator.visualstudio

import com.sos.scheduler.kernel.cplusplus.generator.module.CppModule
import java.io.File

object VisualStudio {
    def updateProjectFiles(dir: File, modules: Seq[CppModule]) {
        VcprojFile.update(dir, modules);
        VcxprojFile.update(dir, modules);
    }
}
