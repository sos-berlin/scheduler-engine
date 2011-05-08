package com.sos.scheduler.engine.kernel.folder;


public class AbsolutePath extends Path {
    public AbsolutePath(String p) {
        super(p);
        assertIsEmptyOrAbsolute();
    }
}
