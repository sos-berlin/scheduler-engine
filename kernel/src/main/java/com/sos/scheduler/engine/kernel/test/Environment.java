package com.sos.scheduler.engine.kernel.test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import org.apache.log4j.Logger;
import static org.apache.commons.io.FileUtils.*;
import static com.google.common.base.Strings.*;
import static com.sos.scheduler.engine.kernel.test.OperatingSystem.*;


public final class Environment {
    private static final String kernelCppDirName = "kernel-cpp";
    private static final String moduleBase = kernelCppDir() + "/"+ bin() + "/" + "scheduler";
    private static final OperatingSystem os = OperatingSystem.singleton;
    private static final File schedulerModuleFile = new File(os.makeModuleFilename(moduleBase)).getAbsoluteFile();
    private static final File schedulerExeFile = new File(os.makeExecutableFilename(moduleBase)).getAbsoluteFile();
    private static final List<String> defaultFiles = Arrays.asList("scheduler.xml", "factory.ini", "sos.ini");
    private static final Logger logger = Logger.getLogger(Environment.class);

    private final File directory;
    private final Object mainObject;
    private final File logDirectory;


    private static File kernelCppDir() {
        File dir = new File(".");
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
        copyResources();
    }


    private static void loadSchedulerModule() {
        System.load(schedulerModuleFile.getPath());
    }

    
//    private static void addSchedulerPathToJavaLibraryPath() {
//        prependJavaLibraryPath(schedulerBinDirectory());
//    }


    private static File schedulerBinDirectory() {
        return schedulerModuleFile.getAbsoluteFile().getParentFile();
    }

    
    private void copyResources() {
        copyDefaultResource();
        copyOtherResources();
    }


    private void copyDefaultResource() {
        for (String n: defaultFiles)  copyResource(n);
    }

    
    private void copyOtherResources() {
        //TODO Wie die Resourcen eines Package auflisten?
        //class.getResource(packageName), file: und jar: unterscheiden, dann direkten Zugriff auf Dateien oder Jar-Elemente.
        //C++/Java-Brücke kann Jar-Elemente auflisten
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
            "-sos.ini=" + new File(directory, "sos.ini").getAbsolutePath(),  // Warum getAbsolutePath? "sos.ini" könnte Windows die sos.ini unter c:\windows finden lassen
            "-ini=" + new File(directory, "factory.ini").getAbsolutePath(),  // Warum getAbsolutePath? "factory.ini" könnte Windows die factory.ini unter c:\windows finden lassen
            "-log-dir=" + logDirectory.getPath(),
            "-log=" + new File(logDirectory, "scheduler.log").getPath(),
            "-java-events"));

        if (OperatingSystem.isUnix) {
            // Damit der Scheduler die libspidermonkey.so aus seinem Programmverzeichnis laden kann
            String varName = OperatingSystem.singleton.getDynamicLibraryEnvironmentVariableName();
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
        } catch(IOException x) { 
            logger.error(x); 
            //throw new RuntimeException(x); 
        }
    }
}
