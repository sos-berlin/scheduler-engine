package com.sos.scheduler.engine.test;

import java.io.File;
import java.net.URL;

import static com.sos.scheduler.engine.kernel.util.Files.copyURLToFile;

public class StandardResourceToFileTransformer implements ResourceToFileTransformer {
    public static final StandardResourceToFileTransformer singleton = new StandardResourceToFileTransformer();

    private StandardResourceToFileTransformer() {}

    @Override public final void transform(URL url, File file) {
        copyURLToFile(url, file);
    }
}
