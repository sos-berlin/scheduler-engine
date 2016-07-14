package com.sos.scheduler.engine.test;

import java.io.File;
import java.net.URL;

import static com.sos.scheduler.engine.common.system.Files.copyURLToFile;
import static com.sos.scheduler.engine.common.system.OperatingSystemJava.isWindows;

public class StandardResourceToFileTransformer implements ResourceToFileTransformer {
    public static final StandardResourceToFileTransformer singleton = new StandardResourceToFileTransformer();

    private StandardResourceToFileTransformer() {}

    @Override public final void transform(URL url, File file) {
        if (isWindows && file.getPath().length() > 259)
            throw new RuntimeException("File path too long (>259) for JobScheduler under Windows: "+ file);
        copyURLToFile(url, file);
    }
}
