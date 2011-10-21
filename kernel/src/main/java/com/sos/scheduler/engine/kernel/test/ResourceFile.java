package com.sos.scheduler.engine.kernel.test;

import java.io.File;

public final class ResourceFile {
    private final File file;
    private final boolean isCopied;

    public ResourceFile(File file, boolean copied) {
        this.file = file;
        isCopied = copied;
    }

    public File getFile() {
        return file;
    }

    public boolean isCopied() {
        return isCopied;
    }
}
