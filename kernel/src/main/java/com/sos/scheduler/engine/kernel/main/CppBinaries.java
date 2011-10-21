package com.sos.scheduler.engine.kernel.main;

import java.io.File;

import com.sos.scheduler.engine.kernel.test.binary.CppBinary;

public interface CppBinaries {
    File directory();
    File file(CppBinary o);
}
