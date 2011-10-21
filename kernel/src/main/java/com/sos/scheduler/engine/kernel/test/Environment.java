package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Strings.nullToEmpty;
import static com.sos.scheduler.engine.kernel.util.Files.makeDirectories;
import static com.sos.scheduler.engine.kernel.util.Files.makeTemporaryDirectory;
import static com.sos.scheduler.engine.kernel.util.Files.removeDirectoryRecursivly;
import static com.sos.scheduler.engine.kernel.util.OperatingSystem.operatingSystem;

import java.io.File;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.main.CppBinaries;
import com.sos.scheduler.engine.kernel.test.binary.CppBinary;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.kernel.util.ResourcePath;

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
/** Stellt eine Konfigurationsumgebung für den Scheduler bereit. */
public final class Environment {
    private static final String configSubdir = "config";
    private static final String logSubdir = "log";
    private final ResourcePath resourcePath;
    private final boolean removeDirectoryOnClose;
    private final File directory;
    private final File configDirectory;
    private final File logDirectory;

    Environment(ResourcePath resourcePath) {
        this(resourcePath, makeTemporaryDirectory(), true);
    }

    Environment(ResourcePath resourcePath, File directory) {
        this(resourcePath, directory, false);
    }

    private Environment(ResourcePath resourcePath, File directory, boolean removeDirectoryOnClose) {
        this.resourcePath = resourcePath;
        this.directory = directory;
        this.removeDirectoryOnClose = removeDirectoryOnClose;
        configDirectory = new File(directory, configSubdir);
        logDirectory = new File(directory, logSubdir);
    }

    void prepare() {
        prepareTemporaryConfigurationDirectory();
    }

    void close() {
        if (removeDirectoryOnClose)
            removeDirectoryRecursivly(directory);
    }

    private void prepareTemporaryConfigurationDirectory() {
        makeDirectories(directory);
        makeDirectories(configDirectory);
        makeDirectories(logDirectory);
        EnvironmentFiles.copy(resourcePath, configDirectory);
    }

    ImmutableList<String> standardArgs(CppBinaries cppBinaries) {
        ImmutableList.Builder<String> result = new ImmutableList.Builder<String>();
        result.add(cppBinaries.file(CppBinary.exeFilename).getPath());
        result.add("-sos.ini=" + new File(configDirectory, "sos.ini").getAbsolutePath());  // Warum getAbsolutePath? "sos.ini" könnte Windows die sos.ini unter c:\windows finden lassen
        result.add("-ini=" + new File(configDirectory, "factory.ini").getAbsolutePath());  // Warum getAbsolutePath? "factory.ini" könnte Windows die factory.ini unter c:\windows finden lassen
        result.add("-log-dir=" + logDirectory.getPath());
        result.add("-log=" + new File(logDirectory, "scheduler.log").getPath());
        result.add("-java-events");
        if (OperatingSystem.isUnix)
            result.add("-env=" + libraryPathEnv(cppBinaries.directory()));
        result.add(configDirectory.getPath());
        return result.build();
    }

    /** Damit der Scheduler die libspidermonkey.so aus seinem Programmverzeichnis laden kann. */
    private static String libraryPathEnv(File directory) {
        String varName = operatingSystem.getDynamicLibraryEnvironmentVariableName();
        String previous = nullToEmpty(System.getenv(varName));
        return varName + "=" + OperatingSystem.concatFileAndPathChain(directory, previous);
    }

    public File directory() {
        return directory;
    }

    public File configDirectory() {
        return configDirectory;
    }

    public File logDirectory() {
        return logDirectory;
    }
}
