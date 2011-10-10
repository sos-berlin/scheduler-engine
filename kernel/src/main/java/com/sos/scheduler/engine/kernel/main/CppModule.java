package com.sos.scheduler.engine.kernel.main;

import java.io.File;

public interface CppModule {
    File moduleFile();
    File exeFile();
    File spidermonkeyModuleDirectory();
}
