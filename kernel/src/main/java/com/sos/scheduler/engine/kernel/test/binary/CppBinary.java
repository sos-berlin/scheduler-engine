package com.sos.scheduler.engine.kernel.test.binary;

import com.sos.scheduler.engine.kernel.util.OperatingSystem;

public enum CppBinary {
    moduleFilename(OperatingSystem.singleton.makeModuleFilename("scheduler")),      // scheduler.dll oder libscheduler.so
    exeFilename(OperatingSystem.singleton.makeExecutableFilename("scheduler"));     // scheduler.exe oder scheduler

    private final String filename;

    CppBinary(String filename) {
        this.filename = filename;
    }

    public String filename() {
        return filename;
    }
}
