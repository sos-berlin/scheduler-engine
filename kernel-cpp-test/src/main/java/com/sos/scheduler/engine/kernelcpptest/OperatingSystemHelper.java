package com.sos.scheduler.engine.kernelcpptest;

import org.apache.log4j.Logger;
import com.google.common.base.Joiner;
import java.io.File;
import static com.google.common.base.Strings.*;


public abstract class OperatingSystemHelper {
    public static final String name = System.getProperty("os.name");
    public static final boolean isWindows = name.startsWith("Windows");
    public static final OperatingSystemHelper singleton = isWindows? new WindowsHelper() : new UnixHelper();
    public static final String javaLibraryPathPropertyName = "java.library.path";
    private static final Logger logger = Logger.getLogger(OperatingSystemHelper.class);
    
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


    public static void prependJavaLibraryPath(File f) {
        String abs = f.getAbsolutePath();
        String a = nullToEmpty(System.getProperty(javaLibraryPathPropertyName));
        String[] b = a.split(File.pathSeparator, 2);
        if (b.length == 0  ||  !b[0].equals(abs)) {
            String c = Joiner.on(File.pathSeparatorChar).skipNulls().join(abs, emptyToNull(a));
            System.setProperty(javaLibraryPathPropertyName, c);
            logger.debug("Property " + javaLibraryPathPropertyName + "=" + c);
        }
    }
}
