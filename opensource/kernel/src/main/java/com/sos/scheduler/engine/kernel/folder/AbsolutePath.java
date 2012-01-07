package com.sos.scheduler.engine.kernel.folder;


public class AbsolutePath extends Path {
    public AbsolutePath(String p) {
        super(p);
        assertIsEmptyOrAbsolute();
    }

    public final String getName() {
        return getString().substring(getString().lastIndexOf('/') + 1);
    }

    /** @param path ist absolut oder relativ zur Wurzel. */
    public static AbsolutePath of(String path) {
        return new AbsolutePath(path.startsWith("/")? path : "/" + path);
    }
}
