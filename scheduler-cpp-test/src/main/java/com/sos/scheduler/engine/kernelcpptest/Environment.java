package com.sos.scheduler.engine.kernelcpptest;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import static org.apache.commons.io.FileUtils.*;


public class Environment {
    private static final File workingDirectory = new File(System.getProperty("user.dir"));
    private static final String moduleBase = "../core-cpp/prod/" + bin() + "/" + "scheduler";
    private static final OperatingSystemHelper os = OperatingSystemHelper.singleton;
    private static final File schedulerModuleFile = new File(workingDirectory, os.makeModuleFilename(moduleBase)).getAbsoluteFile();
    private static final File schedulerExeFile = new File(workingDirectory, os.makeExecutableFilename(moduleBase)).getAbsoluteFile();

    private final Object mainObject;
    final File directory;
    final File factoryIniFile;
    private final File logDirectory;

    
    private static String bin() {
        return OperatingSystemHelper.isWindows? "bind"  // Die scheduler.dll wird nur für die Debug-Variante erzeugt
          : "bin";
    }


    public Environment(Object mainObject) {
        try {
            this.mainObject = mainObject;
            directory = File.createTempFile("sos", ".tmp");
            factoryIniFile = new File(directory, "factory.ini");
            logDirectory = new File(directory, "log");
            prepareConfigurationFiles();
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


    private boolean copyResourceOptional(String name, File destination) {
        try {
            if (destination == null)  destination = new File(directory, name);
            URL url = resourceClass().getResource(name);
            boolean exists = url != null;
            if (exists)
                copyURLToFile(url, destination);
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
