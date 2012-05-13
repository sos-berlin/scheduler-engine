package com.sos.scheduler.engine.test.binary;

import java.io.File;

import com.sos.scheduler.engine.kernel.util.CpuArchitecture;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import org.apache.log4j.Logger;

import com.google.common.base.Preconditions;
import com.sos.scheduler.engine.main.CppBinaries;
import com.sos.scheduler.engine.main.CppBinary;

import static com.google.common.base.Preconditions.checkArgument;
import static com.sos.scheduler.engine.kernel.util.CpuArchitecture.x86;
import static com.sos.scheduler.engine.kernel.util.OperatingSystem.*;

/** Liefert die Binärdateien des Maven-Artefakts kernel-cpp, das in einem Oberverzeichnis stehen muss. */
public final class KernelCppArtifactBinaries implements CppBinaries {
    private static final Logger logger = Logger.getLogger(KernelCppArtifactBinaries.class);
    private static final String kernelCppDirName = "opensource/kernel-cpp";
    private static final String bin = isWindows? cpuArchitecture.visualStudioName() +"/Debug" : "bin"; // Für x86 wird die scheduler.dll wird nur für die Debug-Variante erzeugt

    private final File directory = new File(kernelCppDir(), bin);

    KernelCppArtifactBinaries() {
        checkArgument(directory.isDirectory(), "%s does not exist or is not a directory", directory);
        logger.info("Using JobScheduler binaries in detected artifact directory " + directory);
    }

    private static File kernelCppDir() {
        File dir = new File(".");
        while (dir.exists()) {
            File result = new File(dir, kernelCppDirName);
            if (result.exists()) return result.getAbsoluteFile();
            dir = new File(dir, "..");
        }
        throw new RuntimeException("No parent directory has a subdirectory '"+kernelCppDirName+"'");
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
