package com.sos.scheduler.engine.test;

import static com.google.common.base.Strings.nullToEmpty;
import static com.sos.scheduler.engine.kernel.util.Files.makeDirectories;
import static com.sos.scheduler.engine.kernel.util.Files.makeTemporaryDirectory;
import static com.sos.scheduler.engine.kernel.util.OperatingSystem.operatingSystem;

import java.io.File;

import org.apache.log4j.Logger;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.folder.Path;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.main.CppBinaries;
import com.sos.scheduler.engine.main.CppBinary;

/** Stellt eine Konfigurationsumgebung für den Scheduler bereit.
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
    private static final String configSubdir = "config";
    private static final String logSubdir = "log";

    private final ResourcePath resourcePath;
    private final File directory;
    private final File configDirectory;
    private final File logDirectory;

    Environment(ResourcePath resourcePath, File directory) {
        this.resourcePath = resourcePath;
        this.directory = directory;
        configDirectory = new File(directory, configSubdir);
        logDirectory = new File(directory, logSubdir);
    }

    void prepare() {
        prepareTemporaryConfigurationDirectory();
    }

    private void prepareTemporaryConfigurationDirectory() {
        makeDirectories(directory);
        makeDirectories(configDirectory);
        makeDirectories(logDirectory);
        EnvironmentFiles.copy(resourcePath, configDirectory);
    }

    ImmutableList<String> standardArgs(CppBinaries cppBinaries, String logCategories) {
        ImmutableList.Builder<String> result = new ImmutableList.Builder<String>();
        result.add(cppBinaries.file(CppBinary.exeFilename).getPath());
        result.add("-id=test");
        result.add("-sos.ini=" + sosIniFile());
        result.add("-ini=" + iniFile());
        result.add("-log-dir=" + logDirectory.getPath());
        result.add("-log="+logCategories+">" + new File(logDirectory, "scheduler.log").getPath());
        result.add("-java-events");
        if (OperatingSystem.isUnix)
            result.add("-env=" + libraryPathEnv(cppBinaries.directory()));
        result.add(configDirectory.getPath());
        return result.build();
    }

    public File sosIniFile() {
        return new File(configDirectory, "sos.ini").getAbsoluteFile();
        // Warum getAbsolutePath? "sos.ini" könnte Windows die sos.ini unter c:\windows finden lassen
    }

    public File iniFile() {
        return new File(configDirectory, "factory.ini").getAbsoluteFile();
        // Warum getAbsolutePath? "factory.ini" könnte Windows die factory.ini unter c:\windows finden lassen
    }

    /** Damit der Scheduler die libspidermonkey.so aus seinem Programmverzeichnis laden kann. */
    private static String libraryPathEnv(File directory) {
        String varName = operatingSystem.getDynamicLibraryEnvironmentVariableName();
        String previous = nullToEmpty(System.getenv(varName));
        return varName + "=" + OperatingSystem.concatFileAndPathChain(directory, previous);
    }

    public File fileFromPath(Path p, String suffix) {
        return new File(configDirectory(), p + suffix);
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
