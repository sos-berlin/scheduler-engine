package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Strings.isNullOrEmpty;
import static com.sos.scheduler.engine.kernel.test.OperatingSystem.isWindows;
import static com.sos.scheduler.engine.kernel.test.OperatingSystem.singleton;
import static com.sos.scheduler.engine.kernel.util.Files.makeTemporaryDirectory;

import java.io.File;

import org.apache.log4j.Logger;

import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.main.CppBinaries;

final class TestSchedulerCppBinaries implements CppBinaries {
    private static final Logger logger = Logger.getLogger(TestSchedulerCppBinaries.class);
    private static final String binariesTmpdirPropertyName = "scheduler.binaries.tmpdir";
    private static final String moduleBase = "scheduler";
    private static final String spidermonkeyBase = "spidermonkey";
    private static final String kernelCppPackageDirectory = "com/sos/scheduler/engine/kernelcpp/bin-test";

    private final File directory;
    private ImmutableMap<String,ResourceFile> resourceFiles = null;

    TestSchedulerCppBinaries(File temporaryBinDirectory) {
        this.directory = temporaryBinDirectory.getAbsoluteFile();
    }

    void provideResourcesAsFiles() {
        resourceFiles = ResourcesAsFilesProvider.provideResourcesAsFiles("classpath*:" + kernelCppPackageDirectory + "/*", directory);
    }

    private boolean someResourceHasBeenCopied() {
        for (ResourceFile f: resourceFiles.values())
            if (f.isCopied()) return true;
        return false;
    }

//    public void removeFileCopies() {
//        for (ResourceFile f: resourceFiles.values()) if (f.isCopied()) {
//            File file = newCppBinaries.getFile();
//            boolean ok = file.delete();
//            if (!ok)
//                file.deleteOnExit();
//        }
//    }

    public void removeCopiesOnExit() {
        for (ResourceFile f: resourceFiles.values()) if (f.isCopied()) f.getFile().deleteOnExit();
    }

//    public Iterable<File> createdFiles() {
//        return concat(transform(resourceFiles.values(), new Function<ResourceFile,Iterable<File>>() {
//            @Override public Iterable<File> apply(ResourceFile o) {
//                if (o.isCopied()) return Collections.singleton(o.getFile());
//                else return Collections.emptyList();
//            }
//        }));
//    }

    @Override public File moduleFile() {
        return get(singleton.makeModuleFilename(moduleBase));
    }

    @Override public File exeFile() {
        return get(singleton.makeExecutableFilename(moduleBase));
    }

    @Override public File directory() {
        return get(singleton.makeModuleFilename(spidermonkeyBase)).getParentFile();
    }

    File get(String name) {
        assertResourceFiles();
        ResourceFile result = resourceFiles.get(name);
        if (result == null) throw new RuntimeException("Unknown resource " + name);
        return result.getFile();
    }

    private void assertResourceFiles() {
        if (resourceFiles == null)
            throw new IllegalStateException("provideResourcesAsFiles() not called");
    }

    static CppBinaries cppBinaries() {
        return ThreadSafeInitialization.singleton.cppBinaries;
    }

    public static class ThreadSafeInitialization {
        private static final ThreadSafeInitialization singleton = new ThreadSafeInitialization();
        private final CppBinaries cppBinaries;

        private ThreadSafeInitialization() {
            String d = System.getProperty(binariesTmpdirPropertyName);
            cppBinaries = isNullOrEmpty(d)? newTemporaryCppBinaries() : newCppBinaries(new File(d));
        }

        private static CppBinaries newTemporaryCppBinaries() {
            File dir = makeTemporaryDirectory();
            TestSchedulerCppBinaries result = newCppBinaries(dir);
            if (result.someResourceHasBeenCopied()) warnUndeletable(dir);
            result.removeCopiesOnExit();
            dir.deleteOnExit();
            return result;
        }

        private static TestSchedulerCppBinaries newCppBinaries(File dir) {
            Preconditions.checkArgument(dir.isDirectory(), "%s must be a directory", dir);
            TestSchedulerCppBinaries result = new TestSchedulerCppBinaries(dir);
            result.provideResourcesAsFiles();
            return result;
        }

        private static void warnUndeletable(File dir) {
            if (isWindows)
                logger.warn("Some Scheduler binary files will likely not be deleted in temporary directory "+dir);
        }
    }
}
