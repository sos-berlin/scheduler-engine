package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Strings.nullToEmpty;
import static com.google.common.io.Files.createParentDirs;
import static com.sos.scheduler.engine.kernel.test.OperatingSystem.concatFileAndPathChain;

import java.io.File;
import java.io.IOException;

import org.apache.log4j.Logger;

import com.google.common.collect.ImmutableList;

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

    private final File directory;
    private final File logDirectory;
    private final Package pack;
    private final SchedulerCppModule module = new SchedulerCppModule();

    public Environment(Package pack, File directory) {
        this.directory = directory;
        logDirectory = new File(directory, "log");
        this.pack = pack;
        module.load();
        prepareTemporaryConfigurationDirectory();
        //addSchedulerPathToJavaLibraryPath();    // Für libspidermonkey.so
    }

    private void prepareTemporaryConfigurationDirectory() {
        try {
            createParentDirs(new File(directory, "x"));
            createParentDirs(new File(logDirectory, "x"));
            EnvironmentFiles.copy(pack, directory);
        } catch (IOException e) { throw new RuntimeException(e); }
    }

//    private static void addSchedulerPathToJavaLibraryPath() {
//        prependJavaLibraryPath(binDirectory());
//    }

    public ImmutableList<String> standardArgs() {
        ImmutableList.Builder<String> result = new ImmutableList.Builder<String>();
        result.add(module.exeFile.getPath());
        result.add("-sos.ini=" + new File(directory, "sos.ini").getAbsolutePath());  // Warum getAbsolutePath? "sos.ini" könnte Windows die sos.ini unter c:\windows finden lassen
        result.add("-ini=" + new File(directory, "factory.ini").getAbsolutePath());  // Warum getAbsolutePath? "factory.ini" könnte Windows die factory.ini unter c:\windows finden lassen
        result.add("-log-dir=" + logDirectory.getPath());
        result.add("-log=" + new File(logDirectory, "scheduler.log").getPath());
        result.add("-java-events");
        if (OperatingSystem.isUnix)
            result.add("-env=" + spidermonkeyLibraryPathEnv());
        result.add(directory.getPath());
        return result.build();
    }

    /** Damit der Scheduler die libspidermonkey.so aus seinem Programmverzeichnis laden kann. */
    private String spidermonkeyLibraryPathEnv() {
        String varName = OperatingSystem.singleton.getDynamicLibraryEnvironmentVariableName();
        String previous = nullToEmpty(System.getenv(varName));
        return varName + "=" + concatFileAndPathChain(module.binDirectory(), previous);
    }

    public File getDirectory() {
        return directory;
    }

//    public static Environment of(Package pack) {
//        return new Environment(pack, MavenDirectory.newDirectory());
//    }
//
//    private static class MavenDirectory {
//        private static final ResourceProperties p = new ResourceProperties(resourceUrl(Environment.class, "maven.properties"));
//        private static final File baseDirectory = new File(p.get("project.build.directory"), "SchedulerTest");
//
//        /** Nicht thread-sicher. */
//        private static File newDirectory() {
//            makeDirectory(baseDirectory);
//            for (int i = 1;; i++) {
//                File f = new File(baseDirectory, Integer.toString(i));
//                if (f.exists()) {
//                    makeDirectory(f);
//                    return f;
//                }
//            }
//        }
//    }
}
