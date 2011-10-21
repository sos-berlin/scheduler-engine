package com.sos.scheduler.engine.kernel.main;

import java.io.File;

public interface CppBinaries {
    File moduleFile();
    File exeFile();
    File directory();
}
