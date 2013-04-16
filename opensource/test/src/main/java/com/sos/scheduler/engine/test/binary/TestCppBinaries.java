package com.sos.scheduler.engine.test.binary;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.common.Lazy;
import com.sos.scheduler.engine.main.CppBinaries;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import java.io.File;
import java.io.IOException;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.base.Strings.isNullOrEmpty;
import static com.sos.scheduler.engine.common.system.Files.makeTemporaryDirectory;
import static com.sos.scheduler.engine.common.system.OperatingSystem.isWindows;
import static com.sos.scheduler.engine.kernel.util.Util.ignore;

public final class TestCppBinaries {
    private static final Logger logger = LoggerFactory.getLogger(TestCppBinaries.class);
    private static final String binariesTmpdirPropertyName = "com.sos.scheduler.engine.test.binary.tmpdir";
    private static final PathMatchingResourcePatternResolver resourceResolver = new PathMatchingResourcePatternResolver();
    private static final String kernelCppPackageDirectory = "com/sos/scheduler/engine/kernelcpp/bin-test";
    private static final String resourcePattern = "classpath*:" + kernelCppPackageDirectory + "/*";

    private TestCppBinaries() {}

    public static CppBinaries cppBinaries(CppBinariesDebugMode debugMode) {
        return debugMode == CppBinariesDebugMode.debug? debugCppBinaries.get() : releaseCppBinaries.get();
    }

    private static final Lazy<CppBinaries> debugCppBinaries = new Lazy<CppBinaries>() {
        @Override protected CppBinaries compute() { return newCppBinaries(CppBinariesDebugMode.debug); }
    };
    private static final Lazy<CppBinaries> releaseCppBinaries = new Lazy<CppBinaries>() {
        @Override protected CppBinaries compute() { return newCppBinaries(CppBinariesDebugMode.release); }
    };

    private static CppBinaries newCppBinaries(CppBinariesDebugMode debugMode) {
        ImmutableList<Resource> resources = resources();
        if (resources.isEmpty()) {
            // Das passiert, wenn ohne Maven gebaut wird. So unter der IntelliJ-IDE. Dann greifen wir eben direkt auf die binaries im Dateisystem zu.
            if (!isUnderIDE())
                throw new RuntimeException("Missing kernel-cpp resources while running under Maven");  // Denn nur bei den Ressourcen sind wir über deren Stand sicher.
            return new KernelCppArtifactBinaries(debugMode);
        } else {
            // Wir packen die binaries aus den Ressourcen aus
            String d = System.getProperty(binariesTmpdirPropertyName);
            return isNullOrEmpty(d)? newTemporaryCppBinaries(resources, debugMode)
                    : newExistingCppBinaries(resources, new File(d), debugMode);   // Wir recyceln die vorher mal oder von einem parallel laufenden Test ausgepacken Ressourcen.
        }
    }

    private static boolean isUnderIDE() {
        return System.getProperty("sun.java.command").startsWith("com.intellij.") ||     // IntelliJ IDEA 12.1 kennt die Maven-Properties
            System.getProperty("com.sos.scheduler.engine.test.underMaven") == null;
    }

    private static ImmutableList<Resource> resources() {
        try {
            return ImmutableList.copyOf(resourceResolver.getResources(resourcePattern));
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    private static CppBinaries newTemporaryCppBinaries(ImmutableList<Resource> resources, CppBinariesDebugMode debugMode) {
        File dir = makeTemporaryDirectory();
        ResourceCppBinaries result = newExistingCppBinaries(resources, dir, debugMode);
        if (result.someResourceHasBeenCopied()) warnUndeletable(dir);
        result.removeCopiesOnExit();
        dir.deleteOnExit();
        return result;
    }

    private static ResourceCppBinaries newExistingCppBinaries(ImmutableList<Resource> resources, File dir, CppBinariesDebugMode debugMode) {
        ignore(dir.mkdir());
        File subDir = new File(dir, debugMode.name());
        ignore(subDir.mkdir());
        checkArgument(subDir.isDirectory(), "%s must exist and must be a directory", subDir);
        return new ResourceCppBinaries(resources, subDir);
    }

    private static void warnUndeletable(File dir) {
        if (isWindows)
            logger.warn("Some Scheduler binary files will likely not be deleted in temporary directory {}. Use property {}",
                    dir, binariesTmpdirPropertyName);
    }
}
