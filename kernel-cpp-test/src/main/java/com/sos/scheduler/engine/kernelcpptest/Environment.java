package com.sos.scheduler.engine.kernelcpptest;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import org.apache.log4j.Logger;
import static org.apache.commons.io.FileUtils.*;
import static com.google.common.base.Strings.*;
import static com.sos.scheduler.engine.kernelcpptest.OperatingSystemHelper.*;


public class Environment {
    private static final String kernelCppDirName = "kernel-cpp";
    private static final String moduleBase = kernelCppDir() + "/"+ bin() + "/" + "scheduler";
    private static final OperatingSystemHelper os = OperatingSystemHelper.singleton;
    private static final File schedulerModuleFile = new File(os.makeModuleFilename(moduleBase)).getAbsoluteFile();
    private static final File schedulerExeFile = new File(os.makeExecutableFilename(moduleBase)).getAbsoluteFile();
    private static final Logger logger = Logger.getLogger(Environment.class);

    final File directory;
    final File sosIniFile;
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
            sosIniFile = new File(directory, "sos.ini");
            factoryIniFile = new File(directory, "factory.ini");
            logDirectory = new File(directory, "log");
            prepareConfigurationFiles();
            //addSchedulerPathToJavaLibraryPath();    // Für libspidermonkey.so
        }
        catch(IOException x) { throw new RuntimeException(x); }
    }


    private void prepareConfigurationFiles() {
        loadSchedulerModule();
        directory.delete();
        directory.mkdir();
        logDirectory.mkdir();
        copyResource("scheduler.xml");
        copyResource("sos.ini", sosIniFile);
        copyResource("factory.ini", factoryIniFile);
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


    private void copyResource(String name, File destinationOrNull) {
        try {
            File dest = destinationOrNull != null? destinationOrNull : new File(directory, name);
            URL url = resourceClass().getResource(name);
            if (url == null) {
                url = Environment.class.getResource(name);
                if (url == null)  throw new RuntimeException("Resource '" + name + "' is missing");
            }
            copyURLToFile(url, dest);
        }
        catch(IOException x) { throw new RuntimeException(x); }
    }


    private Class<?> resourceClass() {
        return mainObject.getClass();
    }


    public String[] standardArgs() {
        List<String> result = new ArrayList<String>(Arrays.asList(
            schedulerExeFile.getPath(),
            "-sos.ini=" + sosIniFile.getAbsolutePath(),  // Warum getAbsolutePath? "sos.ini" könnte Windows die sos.ini unter c:\windows finden lassen
            "-ini=" + factoryIniFile.getAbsolutePath(),  // Warum getAbsolutePath? "factory.ini" könnte Windows die factory.ini unter c:\windows finden lassen
            "-log-dir=" + logDirectory.getPath(),
            "-log=" + new File(logDirectory, "scheduler.log").getPath(),
            "-java-events"));

        if (OperatingSystemHelper.isUnix) {
            // Damit der Scheduler die libspidermonkey.so aus seinem Programmverzeichnis laden kann
            String varName = OperatingSystemHelper.singleton.getDynamicLibraryEnvironmentVariableName();
            String p = nullToEmpty(System.getenv(varName));
            String arg = "-env=" + varName + "=" + concatFileAndPathChain(schedulerBinDirectory(), p);
            result.add(arg);
        }

        result.add(directory.getPath());
        return result.toArray(new String[0]);
    }


    public void cleanUp() {
        try {
            if (directory != null)
                deleteDirectory(directory);
        }
        catch(IOException x) { throw new RuntimeException(x); }
    }
}
