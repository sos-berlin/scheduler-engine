package com.sos.scheduler.engine.kernel.test;

import org.springframework.core.io.Resource;
import com.google.common.base.Joiner;
import com.google.common.base.Splitter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import org.apache.log4j.Logger;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;
import static com.google.common.base.Strings.*;
import static com.google.common.io.Files.*;
import static com.sos.scheduler.engine.kernel.test.OperatingSystem.*;
import static org.apache.commons.io.FileUtils.copyURLToFile;
import static java.util.Arrays.asList;

/**
 * \file Environment.java
 * \brief Build the environment for the scheduler binary 
 *  
 * \class Environment
 * \brief Build the environment for the scheduler binary 
 * 
 * \details
 *
 * \code
  \endcode
 *
 * \version 1.0 - 31.08.2011 10:11:54
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public final class Environment {

	private static final Logger logger = Logger.getLogger(Environment.class);
    private static final String kernelCppDirName = "kernel-cpp";
    private static final String moduleBase = schedulerLibDir() + "/" + "scheduler";
//    private static final String moduleBase = kernelCppDir() + "/"+ bin() + "/" + "scheduler";
    private static final OperatingSystem os = OperatingSystem.singleton;
    private static final File schedulerModuleFile = new File(os.makeModuleFilename(moduleBase)).getAbsoluteFile();
    private static final File schedulerExeFile = new File(os.makeExecutableFilename(moduleBase)).getAbsoluteFile();
    private static final List<String> defaultFiles = Arrays.asList("scheduler.xml", "factory.ini", "sos.ini");

    private final Iterable<String> otherResourceNames;
    private final File directory;
    private final Object mainObject;
    private final File logDirectory;

    public Environment(Object mainObject, Iterable<String> otherResourceNames) {
        try {
            this.otherResourceNames = otherResourceNames;
            this.mainObject = mainObject;
            directory = File.createTempFile("sos", ".tmp");
            logDirectory = new File(directory, "log");
            prepareConfigurationFiles();
            //addSchedulerPathToJavaLibraryPath();    // Für libspidermonkey.so
        }
        catch(IOException x) { throw new RuntimeException(x); }
    }

    /**
     * \brief Determine the library for the scheduler binary
     * \detail
     * For running the JobScheduler in a Java Environment it is neccessary to know where the binary of the 
     * Scheduler kernel is stored. In a development landscape it is placed normally in the artifakt 'kernel-cpp' 
     * in a subfolder called 'bind'. 
     * Use the Scheduler in a single project, e.g. a test case, the scheduler binary has to place in a subfolder
     * called 'lib'.
     *
     * @return String - the name of the library in which the scheduler binary has to put in
     */
    private static String schedulerLibDir() {
        File result = new File("./target/lib");
        if (result.exists()) {
        	logger.debug("expecting scheduler binary in '" + result + "'.");
        	return result + "";
        }
        logger.info("Subdirectory 'lib' not found.");

        result = kernelCppDir();
        if (result.exists()) {
        	String resultString = result + "/" + bin();
        	logger.debug("expecting scheduler binary in '" + resultString + "'.");
        	return resultString;
        }
        logger.info("Subdirectory '" + result + "' not found.");
        
        String msg = "No location for the scheduler binary found.";
        logger.error(msg);
        throw new RuntimeException(msg);
    }

    private static File kernelCppDir() {
    	File dir = new File(".");
        while (dir.exists()) {
            File result = new File(dir, kernelCppDirName);
            if (result.exists()) return result;
            dir = new File(dir, "..");
        }
        logger.info("No parent directory has a subdirectory '" + kernelCppDirName + "'");
        return dir;
//        throw new RuntimeException("No parent directory has a subdirectory '" + kernelCppDirName + "'");
    }

    
    private static String bin() {
        return isWindows? "bind"  // Die scheduler.dll wird nur für die Debug-Variante erzeugt
          : "bin";
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
        //TODO Wie die Resourcen eines Package auflisten? Mit Spring?
        //class.getResource(packageName), file: und jar: unterscheiden, dann direkten Zugriff auf Dateien oder Jar-Elemente.
        //C++/Java-Brücke kann Jar-Elemente auflisten
        for (String n: otherResourceNames)  copyResource(n);
    }


    private List<Resource> resources() {
        try {
            PathMatchingResourcePatternResolver r = new PathMatchingResourcePatternResolver();
            return asList(r.getResources(resourceDirectory() + "/*.xml"));
        } catch (IOException x) { throw new RuntimeException(x); }
    }


    private String resourceDirectory() {
        List<String> p = new ArrayList<String>();
        for (String s: Splitter.on(resourceClass().getName()).split("."))  p.add(s);
        p.remove(p.size() - 1);
        return Joiner.on("/").join(p);
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
                deleteRecursively(directory);
        } catch(IOException x) { 
            logger.error(x); 
            //throw new RuntimeException(x); 
        }
    }


    public final File getDirectory() {
        return directory;
    }
}
