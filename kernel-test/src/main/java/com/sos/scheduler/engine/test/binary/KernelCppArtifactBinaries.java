package com.sos.scheduler.engine.test.binary;

import static com.sos.scheduler.engine.kernel.util.OperatingSystem.isWindows;

import java.io.File;

import org.apache.log4j.Logger;

import com.google.common.base.Preconditions;
import com.sos.scheduler.engine.kernel.main.CppBinaries;
import com.sos.scheduler.engine.kernel.main.CppBinary;

/** Liefert die Binärdateien des Maven-Artefakts kernel-cpp, das in einem Oberverzeichnis stehen muss. */
public final class KernelCppArtifactBinaries implements CppBinaries {
    private static final Logger logger = Logger.getLogger(KernelCppArtifactBinaries.class);
    private static final String kernelCppDirName = "kernel-cpp";
    private static final String bin = isWindows? "bind" : "bin"; // Die scheduler.dll wird nur für die Debug-Variante erzeugt

    private final File directory = new File(kernelCppDir(), bin);

    KernelCppArtifactBinaries() {
        Preconditions.checkArgument(directory.isDirectory(), "%s does not exist or is not a directory", directory);
        logger.debug("Using scheduler binaries in detected artifact directory "+directory);
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
