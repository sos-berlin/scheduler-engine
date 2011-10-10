package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Strings.nullToEmpty;
import static com.google.common.io.Files.createParentDirs;
import static com.sos.scheduler.engine.kernel.test.OperatingSystem.concatFileAndPathChain;
import static com.sos.scheduler.engine.kernel.util.Files.makeTemporaryDirectory;
import static com.sos.scheduler.engine.kernel.util.Files.removeDirectoryRecursivly;

import java.io.File;
import java.io.IOException;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.main.CppModule;

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
final class Environment {
    private final CppModule module;
    private final Package pack;
    private final boolean directoryIsTemporary;
    private final File directory;
    private final File logDirectory;

    Environment(CppModule module, Package pack) {
        this(module, pack, makeTemporaryDirectory(), true);
    }

    Environment(CppModule module, Package pack, File directory) {
        this(module, pack, directory, false);
    }

    private Environment(CppModule module, Package pack, File directory, boolean isTemporary) {
        this.module = module;
        this.pack = pack;
        this.directory = directory;
        directoryIsTemporary = isTemporary;
        logDirectory = new File(directory, "log");
    }

    void start() {
        prepareTemporaryConfigurationDirectory();
    }

    void close() {
        if (directoryIsTemporary)
            removeDirectoryRecursivly(directory);
    }

    private void prepareTemporaryConfigurationDirectory() {
        try {
            createParentDirs(new File(directory, "x"));
            createParentDirs(new File(logDirectory, "x"));
            EnvironmentFiles.copy(pack, directory);
        } catch (IOException e) { throw new RuntimeException(e); }
    }

    ImmutableList<String> standardArgs() {
        ImmutableList.Builder<String> result = new ImmutableList.Builder<String>();
        result.add(module.exeFile().getPath());
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
        return varName + "=" + concatFileAndPathChain(module.spidermonkeyModuleDirectory(), previous);
    }

    public File getDirectory() {
        return directory;
    }
}
