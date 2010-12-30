package com.sos.scheduler.engine.kernelcpptest;

import java.io.File;


public abstract class OperatingSystemHelper {
    public static final String name = System.getProperty("os.name");
    public static final boolean isWindows = name.startsWith("Windows");
    public static final OperatingSystemHelper singleton = isWindows? new WindowsHelper() : new UnixHelper();
    
    public abstract String makeModuleFilename(String name);
    public abstract String makeExecutableFilename(String name);
    

    public static class WindowsHelper extends OperatingSystemHelper {
        @Override public String makeModuleFilename(String name) {
            return name + ".dll";
        }

        @Override public String makeExecutableFilename(String name) {
            return name + ".exe";
        }
    }
    

    public static class UnixHelper extends OperatingSystemHelper {
        @Override public String makeModuleFilename(String name) {
            File file = new File(name);
            return new File(file.getParent(), "lib" + file.getName() + ".so").getPath();
        }

        @Override public String makeExecutableFilename(String name) {
            return name;
        }
    }
}
