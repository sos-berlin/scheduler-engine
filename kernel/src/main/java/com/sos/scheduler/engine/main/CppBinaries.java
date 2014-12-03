package com.sos.scheduler.engine.main;

import java.io.File;

public interface CppBinaries {
    File directory();
    File file(CppBinary o);
}
