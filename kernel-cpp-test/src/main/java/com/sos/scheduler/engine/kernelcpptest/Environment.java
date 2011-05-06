package com.sos.scheduler.engine.kernelcpptest;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import org.apache.log4j.Logger;
import static org.apache.commons.io.FileUtils.*;
import static com.sos.scheduler.engine.kernelcpptest.OperatingSystemHelper.*;


public class Environment {
    private static final String kernelCppDirName = "kernel-cpp";
    private static final String moduleBase = kernelCppDir() + "/"+ bin() + "/" + "scheduler";
    private static final OperatingSystemHelper os = OperatingSystemHelper.singleton;
    private static final File schedulerModuleFile = new File(os.makeModuleFilename(moduleBase)).getAbsoluteFile();
    private static final File schedulerExeFile = new File(os.makeExecutableFilename(moduleBase)).getAbsoluteFile();
    private static final Logger logger = Logger.getLogger(Environment.class);

    final File directory;
    final File factoryIniFile;
    private final Object mainObject;
    private final File logDirectory;


    private static File kernelCppDir() {
        File dir = new File("..");
        while (dir.exists()) {
            File result = new File(dir, kernelCppDirName);
            if (result.exists()) return result;
            dir = new File(dir, "..");
        }
        throw new RuntimeException("No parent directory has a subdirectory '" + kernelCppDirName + "'");
    }

    
    private static String bin() {
        return isWindows? "bind"  // Die scheduler.dll wird nur für die Debug-Variante erzeugt
          : "bin";
    }


    public Environment(Object mainObject) {
        try {
            this.mainObject = mainObject;
            directory = File.createTempFile("sos", ".tmp");
            factoryIniFile = new File(directory, "factory.ini");
            logDirectory = new File(directory, "log");
            prepareConfigurationFiles();
            addSchedulerPathToJavaLibraryPath();    // Für libspidermonkey.so
        }
        catch(IOException x) { throw new RuntimeException(x); }
    }


    private void prepareConfigurationFiles() {
        loadSchedulerModule();
        directory.delete();
        directory.mkdir();
        logDirectory.mkdir();
        copyResource("scheduler.xml");
        copyResourceOptional("factory.ini", factoryIniFile);
    }


    private static void loadSchedulerModule() {
        System.load(schedulerModuleFile.getPath());
    }

    
    private static void addSchedulerPathToJavaLibraryPath() {
        prependJavaLibraryPath(schedulerBinDirectory());
    }


    private static File schedulerBinDirectory() {
        return schedulerModuleFile.getAbsoluteFile().getParentFile();
    }

    
    private void copyResource(String name) {
        copyResource(name, null);
    }


    private void copyResource(String name, File destination) {
        boolean exists = copyResourceOptional(name, destination);
        if (!exists)  throw new IllegalStateException("Resource '" + name + "' is missing in package " + resourceClass().getPackage().getName());
    }

    
    private void copyResourceOptional(String name) {
        copyResourceOptional(name, null);
    }


    private boolean copyResourceOptional(String name, File destinationOrNull) {
        try {
            File dest = destinationOrNull != null? destinationOrNull : new File(directory, name);
            URL url = resourceClass().getResource(name);
            boolean exists = url != null;
            if (exists)
                copyURLToFile(url, dest);
            return exists;
        }
        catch(IOException x) { throw new RuntimeException(x); }
    }


    private Class<?> resourceClass() {
        return mainObject.getClass();
    }


    public String[] standardArgs() {
        return new String[]{
            schedulerExeFile.getPath(),
            "-ini=" + factoryIniFile.getPath(),
            "-log-dir=" + logDirectory.getPath(),
            "-log=" + new File(logDirectory, "scheduler.log").getPath(),
            "-java-events",
            directory.getPath() };
    }


    public void cleanUp() {
        try {
            if (directory != null)
                deleteDirectory(directory);
        }
        catch(IOException x) { throw new RuntimeException(x); }
    }
}
