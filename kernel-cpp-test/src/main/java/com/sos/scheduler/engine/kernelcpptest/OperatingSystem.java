package com.sos.scheduler.engine.kernelcpptest;

import com.google.common.base.Joiner;
import java.io.File;
import org.apache.log4j.Logger;
import static com.google.common.base.Strings.*;
import static com.google.common.collect.ObjectArrays.*;


public abstract class OperatingSystem {
    public static final String name = System.getProperty("os.name");
    public static final boolean isWindows = name.startsWith("Windows");
    public static final boolean isUnix = !isWindows;
    public static final OperatingSystem singleton = isWindows? new Windows() : new Unix();
    public static final String javaLibraryPathPropertyName = "java.library.path";
    private static final Logger logger = Logger.getLogger(OperatingSystem.class);
    
    public abstract String makeModuleFilename(String name);
    public abstract String makeExecutableFilename(String name);
    public abstract String getDynamicLibraryEnvironmentVariableName();
    

    public static class Windows extends OperatingSystem {
        @Override public String makeModuleFilename(String name) {
            return name + ".dll";
        }

        @Override public String makeExecutableFilename(String name) {
            return name + ".exe";
        }

        @Override public String getDynamicLibraryEnvironmentVariableName() {
            return "PATH";
        }
    }
    

    public static class Unix extends OperatingSystem {
        @Override public String makeModuleFilename(String name) {
            File file = new File(name);
            return new File(file.getParent(), "lib" + file.getName() + ".so").getPath();
        }

        @Override public String makeExecutableFilename(String name) {
            return name;
        }

        @Override public String getDynamicLibraryEnvironmentVariableName() {
            return "LD_LIBRARY_PATH";
        }
    }


    public static void prependJavaLibraryPath(File f) {
        String a = System.getProperty(javaLibraryPathPropertyName);
        String c = concatFileAndPathChain(f, nullToEmpty(a));
        if (!c.equals(a)) {
            System.setProperty(javaLibraryPathPropertyName, c);
            logger.debug("Property " + javaLibraryPathPropertyName + "=" + c);
        }
    }


    public static String concatFileAndPathChain(File f, String pathChain) {
        String abs = f.getAbsolutePath();
        String[] b = pathChain.split(File.pathSeparator);
        for (int i = 0; i < b.length; i++)  if (b[i].isEmpty() || b[i].equals(abs.toString()))  b[i] = null;
        return Joiner.on(File.pathSeparatorChar).skipNulls().join(concat(abs, b));
    }
}
