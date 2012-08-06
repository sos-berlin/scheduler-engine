package com.sos.scheduler.engine.test.binary;

import static com.google.common.base.Strings.isNullOrEmpty;
import static com.sos.scheduler.engine.kernel.util.OperatingSystem.isWindows;
import static com.sos.scheduler.engine.kernel.util.Files.makeTemporaryDirectory;
import static com.sos.scheduler.engine.kernel.util.Util.ignore;

import java.io.File;
import java.io.IOException;

import org.apache.log4j.Logger;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.main.CppBinaries;
import com.sos.scheduler.engine.kernel.util.Lazy;

public final class TestCppBinaries {
    private static final Logger logger = Logger.getLogger(TestCppBinaries.class);
    private static final String binariesTmpdirPropertyName = "com.sos.scheduler.engine.test.binary.tmpdir";
    private static final PathMatchingResourcePatternResolver resourceResolver = new PathMatchingResourcePatternResolver();
    private static final String kernelCppPackageDirectory = "com/sos/scheduler/engine/kernelcpp/bin-test";
    private static final String resourcePattern = "classpath*:" + kernelCppPackageDirectory + "/*";

    private TestCppBinaries() {}

    public static CppBinaries cppBinaries() {
        return cppBinaries.get();
    }

    private static final Lazy<CppBinaries> cppBinaries = new Lazy<CppBinaries>() {
        @Override protected CppBinaries compute() { return newCppBinaries(); }
    };

    private static CppBinaries newCppBinaries() {
        ImmutableList<Resource> resources = resources();
        if (resources.isEmpty()) {
            // Das passiert, wenn ohne Maven gebaut wird. So unter der IntelliJ-IDE. Dann greifen wir eben direkt auf die binaries im Dateisystem zu.
            if (System.getProperty("com.sos.scheduler.engine.test.underMaven") != null)
                throw new RuntimeException("Missing kernel-cpp resources while running under Maven");  // Denn nur bei den Ressourcen sind wir über deren Stand sicher.
            return new KernelCppArtifactBinaries();
        } else {
            // Wir packen die binaries aus den Ressourcen aus
            String d = System.getProperty(binariesTmpdirPropertyName);
            return isNullOrEmpty(d)? newTemporaryCppBinaries(resources)
                    : newExistingCppBinaries(resources, new File(d));   // Wir recyceln die vorher mal ausgepacken Ressourcen.
        }
    }

    private static ImmutableList<Resource> resources() {
        try {
            return ImmutableList.copyOf(resourceResolver.getResources(resourcePattern));
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    private static CppBinaries newTemporaryCppBinaries(ImmutableList<Resource> resources) {
        File dir = makeTemporaryDirectory();
        ResourceCppBinaries result = newExistingCppBinaries(resources, dir);
        if (result.someResourceHasBeenCopied()) warnUndeletable(dir);
        result.removeCopiesOnExit();
        dir.deleteOnExit();
        return result;
    }

    private static ResourceCppBinaries newExistingCppBinaries(ImmutableList<Resource> resources, File dir) {
        ignore(dir.mkdir());
        Preconditions.checkArgument(dir.isDirectory(), "%s must exist and must be a directory", dir);
        return new ResourceCppBinaries(resources, dir);
    }

    private static void warnUndeletable(File dir) {
        if (isWindows)
            logger.warn("Some Scheduler binary files will likely not be deleted in temporary directory "+dir+"." +
                    " Use property "+binariesTmpdirPropertyName);
    }
}
