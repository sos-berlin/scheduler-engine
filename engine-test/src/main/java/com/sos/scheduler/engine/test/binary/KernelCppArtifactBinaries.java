package com.sos.scheduler.engine.test.binary;

import com.sos.scheduler.engine.main.CppBinaries;
import com.sos.scheduler.engine.main.CppBinary;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

import static com.google.common.base.Preconditions.checkArgument;
import static com.sos.scheduler.engine.common.system.OperatingSystemJava.cpuArchitecture;
import static com.sos.scheduler.engine.common.system.OperatingSystemJava.isWindows;
import static com.sos.scheduler.engine.test.binary.CppBinariesDebugMode.Debug;

/** Liefert die Binärdateien des Maven-Artefakts kernel-cpp, das in einem Oberverzeichnis stehen muss. */
public final class KernelCppArtifactBinaries implements CppBinaries {
    private static final Logger logger = LoggerFactory.getLogger(KernelCppArtifactBinaries.class);
    private static final String kernelCppDirName = "engine/engine-kernelcpp";

    private final File directory;

    KernelCppArtifactBinaries(CppBinariesDebugMode debugMode) {
        String bin = isWindows?
                cpuArchitecture.visualStudioName() +"/"+ (debugMode == Debug? "Debug" : "Release") :
                cpuArchitecture.officialName() +"/Release";
        directory = new File(kernelCppDir(), bin);
        checkArgument(directory.isDirectory(), "%s does not exist or is not a directory", directory);
        logger.warn("Using JobScheduler binaries detected in artifact directory {}", directory);
    }

    private static File kernelCppDir() {
        File start = new File(".").getAbsoluteFile().getParentFile();
        File dir = start;
        while (dir != null && dir.exists()) {
            File result = new File(dir, kernelCppDirName);
            if (result.exists()) return result.getAbsoluteFile();
            dir = dir.getParentFile();
        }
        throw new RuntimeException("No parent directory has a subdirectory '"+kernelCppDirName+"', starting from "+start);
    }

    @Override public File directory() {
        return directory;
    }

    @Override public File file(CppBinary o) {
        File result = new File(directory, o.filename());
        if (!result.exists())  throw new IllegalStateException("Missing file in "+kernelCppDirName+": "+result);
        return result;
    }
}
