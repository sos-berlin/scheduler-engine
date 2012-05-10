package com.sos.scheduler.engine.test;

import java.io.File;
import java.net.URL;

public interface ResourceToFileTransformer {
    void transform(URL url, File file);
}
