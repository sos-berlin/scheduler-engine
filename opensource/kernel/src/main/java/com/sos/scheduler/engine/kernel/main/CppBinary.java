package com.sos.scheduler.engine.kernel.main;

import static com.sos.scheduler.engine.kernel.util.OperatingSystem.operatingSystem;

public enum CppBinary {
    moduleFilename(operatingSystem.makeModuleFilename("scheduler")),      // scheduler.dll oder libscheduler.so
    exeFilename(operatingSystem.makeExecutableFilename("scheduler"));     // scheduler.exe oder scheduler

    private final String filename;

    CppBinary(String filename) {
        this.filename = filename;
    }

    public String filename() {
        return filename;
    }
}
