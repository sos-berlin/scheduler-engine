package com.sos.scheduler.engine.test.binary;

import java.io.File;

final class ResourceFile {
    private final File file;
    private final boolean isCopied;

    ResourceFile(File file, boolean copied) {
        this.file = file;
        isCopied = copied;
    }

    File getFile() {
        return file;
    }

    boolean isCopied() {
        return isCopied;
    }
}
